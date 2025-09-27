# ===========================================
# BILL SPLITTER - MAKEFILE
# ===========================================

.PHONY: help setup build up down logs clean test

# ===========================================
# BILL SPLITTER - MAKEFILE
# ===========================================

.PHONY: help setup build up down logs clean test detect-ip

# Default target
help:
	@echo "Bill Splitter - Available commands:"
	@echo ""
	@echo "  setup     - Initial project setup (copy .env, detect IP)"
	@echo "  detect-ip - Auto-detect and set host IP"
	@echo "  build     - Build all Docker images"
	@echo "  up        - Start all services in development mode"
	@echo "  up-prod   - Start all services in production mode"
	@echo "  down      - Stop all services"
	@echo "  logs      - Show logs from all services"
	@echo "  clean     - Clean up containers, volumes, and images"
	@echo "  test      - Run tests"
	@echo "  status    - Show status of all services"
	@echo ""

# Auto-detect host IP
detect-ip:
	@echo "🔍 Detecting host IP address..."
	@HOST_IP=$(ip route get 8.8.8.8 2>/dev/null | awk '{print $7; exit}' || hostname -I | awk '{print $1}' || echo "127.0.0.1"); \
	echo "📍 Detected IP: $HOST_IP"; \
	if [ -f .env ]; then \
		sed -i "s/^HOST_IP=.*/HOST_IP=$HOST_IP/" .env; \
	else \
		echo "HOST_IP=$HOST_IP" >> .env; \
	fi; \
	echo "✅ Updated .env with HOST_IP=$HOST_IP"

# Initial setup with IP detection
setup:
	@echo "🚀 Setting up Bill Splitter..."
	@if [ ! -f .env ]; then \
		cp .env.example .env; \
		echo "✅ Created .env file from .env.example"; \
	else \
		echo "✅ .env file already exists"; \
	fi
	@$(MAKE) detect-ip
	@echo "🔧 Setup complete!"
	@echo ""
	@echo "🎯 Next steps:"
	@echo "  1. Run: make build"
	@echo "  2. Run: make up"
	@echo "  3. Test mobile app at: exp://$(grep HOST_IP .env | cut -d'=' -f2):8081"

# Build all images
build:
	@echo "🔨 Building Docker images..."
	docker-compose build --no-cache
	@echo "✅ Build complete!"

# Start development environment
up:
	@echo "🚀 Starting development environment..."
	docker-compose -f docker-compose.yml -f docker-compose.dev.yml up -d
	@echo "✅ All services started!"
	@echo ""
	@echo "📍 Services running at:"
	@echo "   API Gateway:  http://localhost:8000"
	@echo "   Auth Service: http://localhost:8001"
	@echo "   Bill Service: http://localhost:8002"
	@echo "   Frontend:     http://localhost:8081"
	@echo "   Database:     localhost:5432"
	@echo "   Redis:        localhost:6379"

# Start production environment
up-prod:
	@echo "🚀 Starting production environment..."
	docker-compose up -d
	@echo "✅ Production services started!"

# Stop all services
down:
	@echo "🛑 Stopping all services..."
	docker-compose -f docker-compose.yml -f docker-compose.dev.yml down
	@echo "✅ All services stopped!"

# Show logs
logs:
	docker-compose -f docker-compose.yml -f docker-compose.dev.yml logs -f

# Show status
status:
	@echo "📊 Service Status:"
	@docker-compose ps

# Clean up everything
clean:
	@echo "🧹 Cleaning up..."
	docker-compose -f docker-compose.yml -f docker-compose.dev.yml down -v --remove-orphans
	docker system prune -f
	docker volume prune -f
	@echo "✅ Cleanup complete!"

# Run tests
test:
	@echo "🧪 Running tests..."
	@echo "Tests will be implemented later..."

# Development helpers
logs-api:
	docker-compose logs -f api-gateway

logs-auth:
	docker-compose logs -f auth-service

logs-bill:
	docker-compose logs -f bill-service

logs-db:
	docker-compose logs -f postgres

logs-redis:
	docker-compose logs -f redis