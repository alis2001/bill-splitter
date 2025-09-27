const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const rateLimit = require('express-rate-limit');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = process.env.PORT || 8000;
const HOST = process.env.HOST || '0.0.0.0';

const AUTH_SERVICE_URL = process.env.AUTH_SERVICE_URL || 'http://auth-service:8001';
const BILL_SERVICE_URL = process.env.BILL_SERVICE_URL || 'http://bill-service:8002';
const CONTACTS_SERVICE_URL = process.env.CONTACTS_SERVICE_URL || 'http://contacts-service:8003';
const RATE_LIMIT_WINDOW_MS = parseInt(process.env.RATE_LIMIT_WINDOW_MS) || 900000;
const RATE_LIMIT_MAX = parseInt(process.env.RATE_LIMIT_MAX) || 100;
const CORS_ORIGIN = process.env.CORS_ORIGIN || '*';

app.use(helmet({
  contentSecurityPolicy: false,
  crossOriginEmbedderPolicy: false
}));

app.use(cors({
  origin: CORS_ORIGIN === '*' ? true : CORS_ORIGIN.split(','),
  credentials: true,
  methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
  allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With']
}));

app.use(morgan('combined'));

const authProxy = createProxyMiddleware('/api/auth', {
  target: AUTH_SERVICE_URL,
  changeOrigin: true,
  timeout: 30000,
  pathRewrite: {
    '^/api/auth': ''
  },
  onError: (err, req, res) => {
    console.log('Auth proxy error:', err.message);
    res.status(500).json({ error: 'Proxy error' });
  },
  onProxyReq: (proxyReq, req, res) => {
    console.log('Proxying auth request:', req.method, req.url);
  }
});

const billProxy = createProxyMiddleware('/api/bills', {
  target: BILL_SERVICE_URL,
  changeOrigin: true,
  pathRewrite: {
    '^/api/bills': ''
  }
});

const contactsProxy = createProxyMiddleware('/api/contacts', {
  target: CONTACTS_SERVICE_URL,
  changeOrigin: true,
  pathRewrite: {
    '^/api/contacts': ''
  }
});

// MOVE THESE LINES UP - BEFORE app.use(express.json())
app.use('/api/auth', authProxy);
app.use('/api/bills', billProxy);
app.use('/api/contacts', contactsProxy);

// THEN add body parsing AFTER proxies
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

const limiter = rateLimit({
  windowMs: RATE_LIMIT_WINDOW_MS,
  max: RATE_LIMIT_MAX,
  message: {
    error: 'Too many requests from this IP, please try again later.',
    retryAfter: Math.ceil(RATE_LIMIT_WINDOW_MS / 1000)
  },
  standardHeaders: true,
  legacyHeaders: false,
});
app.use(limiter);

app.get('/health', (req, res) => {
  res.status(200).json({
    status: 'healthy',
    service: 'API Gateway',
    timestamp: new Date().toISOString(),
    version: '1.0.0',
    uptime: process.uptime(),
    environment: process.env.NODE_ENV || 'development'
  });
});

app.get('/', (req, res) => {
  res.json({
    message: 'Bill Splitter API Gateway',
    version: '1.0.0',
    services: {
      auth: '/api/auth/*',
      bills: '/api/bills/*',
      contacts: '/api/contacts/*'
    },
    endpoints: {
      health: '/health',
      docs: '/docs'
    }
  });
});

app.use('*', (req, res) => {
  res.status(404).json({
    error: 'Not Found',
    message: `Route ${req.originalUrl} not found`,
    availableRoutes: [
      'GET /',
      'GET /health',
      'POST /api/auth/register',
      'POST /api/auth/login',
      'GET /api/bills',
      'POST /api/bills'
    ]
  });
});

app.use((err, req, res, next) => {
  console.error('Gateway Error:', err);
  res.status(500).json({
    error: 'Internal Server Error',
    message: process.env.NODE_ENV === 'development' ? err.message : 'Something went wrong'
  });
});

app.listen(PORT, HOST, () => {
  console.log(`🚀 API Gateway running on http://${HOST}:${PORT}`);
  console.log(`📊 Environment: ${process.env.NODE_ENV || 'development'}`);
  console.log(`🔗 Auth Service: ${AUTH_SERVICE_URL}`);
  console.log(`🔗 Bill Service: ${BILL_SERVICE_URL}`);
  console.log(`⚡ Rate Limit: ${RATE_LIMIT_MAX} requests per ${RATE_LIMIT_WINDOW_MS/1000}s`);
  console.log(`🔗 Contacts Service: ${CONTACTS_SERVICE_URL}`);
});

process.on('SIGTERM', () => {
  console.log('📴 SIGTERM received, shutting down gracefully');
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('📴 SIGINT received, shutting down gracefully');
  process.exit(0);
});