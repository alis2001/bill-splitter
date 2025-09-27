const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const rateLimit = require('express-rate-limit');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = process.env.PORT || 8000;
const HOST = process.env.HOST || '0.0.0.0';

// Environment variables
const AUTH_SERVICE_URL = process.env.AUTH_SERVICE_URL || 'http://auth-service:8001';
const BILL_SERVICE_URL = process.env.BILL_SERVICE_URL || 'http://bill-service:8002';
const RATE_LIMIT_WINDOW_MS = parseInt(process.env.RATE_LIMIT_WINDOW_MS) || 900000; // 15 minutes
const RATE_LIMIT_MAX = parseInt(process.env.RATE_LIMIT_MAX) || 100;
const CORS_ORIGIN = process.env.CORS_ORIGIN || '*';

// Security middleware
app.use(helmet({
  contentSecurityPolicy: false,
  crossOriginEmbedderPolicy: false
}));

// CORS configuration
app.use(cors({
  origin: CORS_ORIGIN === '*' ? true : CORS_ORIGIN.split(','),
  credentials: true,
  methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
  allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With']
}));

// Logging
app.use(morgan('combined'));

// Body parsing
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

// Rate limiting
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

// Health check endpoint
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

// API Gateway routes
app.get('/', (req, res) => {
  res.json({
    message: 'Bill Splitter API Gateway',
    version: '1.0.0',
    services: {
      auth: '/api/auth/*',
      bills: '/api/bills/*'
    },
    endpoints: {
      health: '/health',
      docs: '/docs'
    }
  });
});

// Proxy middleware for Auth Service
const authProxy = createProxyMiddleware({
  target: AUTH_SERVICE_URL,
  changeOrigin: true,
  pathRewrite: {
    '^/api/auth': ''
  },
  onError: (err, req, res) => {
    console.error('Auth Service Proxy Error:', err.message);
    res.status(503).json({
      error: 'Auth service unavailable',
      message: 'Please try again later'
    });
  },
  onProxyReq: (proxyReq, req, res) => {
    console.log(`Proxying ${req.method} ${req.url} to Auth Service`);
  }
});

// Proxy middleware for Bill Service
const billProxy = createProxyMiddleware({
  target: BILL_SERVICE_URL,
  changeOrigin: true,
  pathRewrite: {
    '^/api/bills': ''
  },
  onError: (err, req, res) => {
    console.error('Bill Service Proxy Error:', err.message);
    res.status(503).json({
      error: 'Bill service unavailable',
      message: 'Please try again later'
    });
  },
  onProxyReq: (proxyReq, req, res) => {
    console.log(`Proxying ${req.method} ${req.url} to Bill Service`);
  }
});

// Apply proxy middleware
app.use('/api/auth', authProxy);
app.use('/api/bills', billProxy);

// 404 handler
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

// Error handler
app.use((err, req, res, next) => {
  console.error('Gateway Error:', err);
  res.status(500).json({
    error: 'Internal Server Error',
    message: process.env.NODE_ENV === 'development' ? err.message : 'Something went wrong'
  });
});

// Start server
app.listen(PORT, HOST, () => {
  console.log(`🚀 API Gateway running on http://${HOST}:${PORT}`);
  console.log(`📊 Environment: ${process.env.NODE_ENV || 'development'}`);
  console.log(`🔗 Auth Service: ${AUTH_SERVICE_URL}`);
  console.log(`🔗 Bill Service: ${BILL_SERVICE_URL}`);
  console.log(`⚡ Rate Limit: ${RATE_LIMIT_MAX} requests per ${RATE_LIMIT_WINDOW_MS/1000}s`);
});

// Graceful shutdown
process.on('SIGTERM', () => {
  console.log('📴 SIGTERM received, shutting down gracefully');
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('📴 SIGINT received, shutting down gracefully');
  process.exit(0);
});