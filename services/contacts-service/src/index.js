const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const { Pool } = require('pg');
const redis = require('redis');
const Joi = require('joi');
const jwt = require('jsonwebtoken');

const app = express();
const PORT = process.env.PORT || 8003;

const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  max: parseInt(process.env.DB_MAX_CONNECTIONS) || 20,
});

const redisClient = redis.createClient({
  socket: {
    host: process.env.REDIS_HOST || 'redis',
    port: process.env.REDIS_PORT || 6379,
  },
  password: process.env.REDIS_PASSWORD,
});

redisClient.connect().catch(console.error);

app.use(helmet());
app.use(cors());
app.use(morgan('combined'));
app.use(express.json({ limit: '10mb' }));

const authenticateToken = async (req, res, next) => {
  try {
    const authHeader = req.headers.authorization;
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({ error: 'Missing or invalid token' });
    }

    const token = authHeader.substring(7);
    const cachedData = await redisClient.get(`token:${token}`);
    
    if (!cachedData) {
      return res.status(401).json({ error: 'Token expired or invalid' });
    }

    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    const { userId } = JSON.parse(cachedData);
    
    req.userId = userId;
    next();
  } catch (error) {
    return res.status(401).json({ error: 'Invalid token' });
  }
};

const requestSchema = Joi.object({
  email: Joi.string().email().required(),
  message: Joi.string().max(500).optional()
});

const searchSchema = Joi.object({
  query: Joi.string().min(2).max(100).required()
});

app.get('/health', (req, res) => {
  res.json({
    status: 'healthy',
    service: 'Contacts Service',
    timestamp: new Date().toISOString(),
    version: '1.0.0'
  });
});

app.post('/request', authenticateToken, async (req, res) => {
  try {
    const { error, value } = requestSchema.validate(req.body);
    if (error) {
      return res.status(400).json({ error: error.details[0].message });
    }

    const { email, message } = value;

    const userResult = await pool.query(
      'SELECT id FROM users WHERE email = $1 AND is_active = true',
      [email]
    );

    if (userResult.rows.length === 0) {
      return res.status(404).json({ error: 'User not found' });
    }

    const requestedId = userResult.rows[0].id;

    if (requestedId === req.userId) {
      return res.status(400).json({ error: 'Cannot send request to yourself' });
    }

    const existingContact = await pool.query(
      'SELECT id FROM contacts WHERE user_id = $1 AND contact_id = $2',
      [req.userId, requestedId]
    );

    if (existingContact.rows.length > 0) {
      return res.status(409).json({ error: 'Already in contacts' });
    }

    const existingRequest = await pool.query(
      'SELECT id FROM contact_requests WHERE requester_id = $1 AND requested_id = $2',
      [req.userId, requestedId]
    );

    if (existingRequest.rows.length > 0) {
      return res.status(409).json({ error: 'Request already sent' });
    }

    const result = await pool.query(
      'INSERT INTO contact_requests (requester_id, requested_id, message) VALUES ($1, $2, $3) RETURNING id, created_at',
      [req.userId, requestedId, message]
    );

    res.status(201).json({
      success: true,
      request: {
        id: result.rows[0].id,
        created_at: result.rows[0].created_at
      }
    });
  } catch (err) {
    console.error('Contact request error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.get('/requests', authenticateToken, async (req, res) => {
  try {
    const result = await pool.query(`
      SELECT cr.id, cr.message, cr.status, cr.created_at,
             u.name, u.family_name, u.email
      FROM contact_requests cr
      JOIN users u ON cr.requester_id = u.id
      WHERE cr.requested_id = $1 AND cr.status = 'pending'
      ORDER BY cr.created_at DESC
    `, [req.userId]);

    res.json({
      success: true,
      requests: result.rows
    });
  } catch (err) {
    console.error('Get requests error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.put('/requests/:id/accept', authenticateToken, async (req, res) => {
  try {
    const requestId = req.params.id;

    const request = await pool.query(
      'SELECT requester_id FROM contact_requests WHERE id = $1 AND requested_id = $2 AND status = $3',
      [requestId, req.userId, 'pending']
    );

    if (request.rows.length === 0) {
      return res.status(404).json({ error: 'Request not found' });
    }

    const requesterId = request.rows[0].requester_id;

    await pool.query('BEGIN');

    await pool.query(
      'UPDATE contact_requests SET status = $1 WHERE id = $2',
      ['accepted', requestId]
    );

    await pool.query(
      'INSERT INTO contacts (user_id, contact_id) VALUES ($1, $2), ($2, $1)',
      [req.userId, requesterId]
    );

    await pool.query('COMMIT');

    res.json({ success: true, message: 'Contact request accepted' });
  } catch (err) {
    await pool.query('ROLLBACK');
    console.error('Accept request error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.put('/requests/:id/reject', authenticateToken, async (req, res) => {
  try {
    const requestId = req.params.id;

    const result = await pool.query(
      'UPDATE contact_requests SET status = $1 WHERE id = $2 AND requested_id = $3 AND status = $4',
      ['rejected', requestId, req.userId, 'pending']
    );

    if (result.rowCount === 0) {
      return res.status(404).json({ error: 'Request not found' });
    }

    res.json({ success: true, message: 'Contact request rejected' });
  } catch (err) {
    console.error('Reject request error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.get('/contacts', authenticateToken, async (req, res) => {
  try {
    const result = await pool.query(`
      SELECT c.id, c.status, c.created_at,
             u.id as contact_id, u.name, u.family_name, u.email
      FROM contacts c
      JOIN users u ON c.contact_id = u.id
      WHERE c.user_id = $1 AND c.status = 'active'
      ORDER BY u.name, u.family_name
    `, [req.userId]);

    res.json({
      success: true,
      contacts: result.rows
    });
  } catch (err) {
    console.error('Get contacts error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.get('/users/search', authenticateToken, async (req, res) => {
  try {
    const { error, value } = searchSchema.validate(req.query);
    if (error) {
      return res.status(400).json({ error: error.details[0].message });
    }

    const { query } = value;
    const searchTerm = `%${query}%`;

    const result = await pool.query(`
      SELECT id, name, family_name, email
      FROM users
      WHERE (name ILIKE $1 OR family_name ILIKE $1 OR email ILIKE $1)
        AND id != $2 AND is_active = true
      LIMIT 10
    `, [searchTerm, req.userId]);

    res.json({
      success: true,
      users: result.rows
    });
  } catch (err) {
    console.error('Search users error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.use('*', (req, res) => {
  res.status(404).json({ error: 'Route not found' });
});

app.use((err, req, res, next) => {
  console.error('Contacts Service Error:', err);
  res.status(500).json({ error: 'Internal server error' });
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`Contacts Service running on port ${PORT}`);
});

process.on('SIGTERM', () => {
  console.log('SIGTERM received, shutting down gracefully');
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('SIGINT received, shutting down gracefully');
  process.exit(0);
});