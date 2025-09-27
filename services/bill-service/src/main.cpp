#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <memory>
#include "database.h"
#include "redis_client.h"
#include "auth_middleware.h"
#include "events_controller.h"
#include "expenses_controller.h"
#include "participants_controller.h"
#include "utils.h"
#include "settlements_controller.h"
using json = nlohmann::json;

int main() {
    const std::string host = "0.0.0.0";
    const int port = std::stoi(getenv("PORT") ? getenv("PORT") : "8002");
    
    auto db = std::make_shared<Database>();
    auto redis = std::make_shared<RedisClient>();
    auto auth = std::make_shared<AuthMiddleware>(redis);
    
    // CONNECT TO SERVICES BEFORE CREATING CONTROLLERS
    if (!db->connect()) {
        std::cerr << "Failed to connect to database" << std::endl;
        return 1;
    }
    
    if (!redis->connect()) {
        std::cerr << "Failed to connect to Redis" << std::endl;
        return 1;
    }
    
    std::cout << "Database and Redis connected successfully" << std::endl;
    
    auto events_controller = std::make_shared<EventsController>(db, auth);
    auto expenses_controller = std::make_shared<ExpensesController>(db, auth);
    auto participants_controller = std::make_shared<ParticipantsController>(db, auth);
    auto settlements_controller = std::make_shared<SettlementsController>(db, auth);

    std::cout << "Controllers initialized successfully" << std::endl;
    
    httplib::Server server;
    
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << req.method << " " << req.path << " " << res.status << std::endl;
    });
    
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json response = {
            {"status", "healthy"},
            {"service", "Bill Service"},
            {"timestamp", getCurrentTimestamp()},
            {"version", "1.0.0"}
        };
        res.set_content(response.dump(), "application/json");
    });
    
    server.Get("/test", [](const httplib::Request&, httplib::Response& res) {
        json response = {{"message", "test route works"}};
        res.set_content(response.dump(), "application/json");
    });

    server.Get("/events", [events_controller](const httplib::Request& req, httplib::Response& res) {
        events_controller->getEvents(req, res);
    });

    
    server.Post("/events", [events_controller](const httplib::Request& req, httplib::Response& res) {
        events_controller->createEvent(req, res);
    });
    
    server.Get("/events/([0-9a-fA-F-]+)", [events_controller](const httplib::Request& req, httplib::Response& res) {
        events_controller->getEvent(req, res);
    });
    
    server.Put("/events/([0-9a-fA-F-]+)", [events_controller](const httplib::Request& req, httplib::Response& res) {
        events_controller->updateEvent(req, res);
    });
    
    server.Delete("/events/([0-9a-fA-F-]+)", [events_controller](const httplib::Request& req, httplib::Response& res) {
        events_controller->deleteEvent(req, res);
    });
    
    // Expenses routes
    server.Get("/events/([0-9a-fA-F-]+)/expenses", [expenses_controller](const httplib::Request& req, httplib::Response& res) {
        expenses_controller->getExpenses(req, res);
    });
    
    server.Post("/events/([0-9a-fA-F-]+)/expenses", [expenses_controller](const httplib::Request& req, httplib::Response& res) {
        expenses_controller->createExpense(req, res);
    });
    
    server.Get("/events/([0-9a-fA-F-]+)/expenses/([0-9a-fA-F-]+)", [expenses_controller](const httplib::Request& req, httplib::Response& res) {
        expenses_controller->getExpense(req, res);
    });
    
    server.Delete("/events/([0-9a-fA-F-]+)/expenses/([0-9a-fA-F-]+)", [expenses_controller](const httplib::Request& req, httplib::Response& res) {
        expenses_controller->deleteExpense(req, res);
    });
    
    // Participants routes
    server.Get("/events/([0-9a-fA-F-]+)/participants", [participants_controller](const httplib::Request& req, httplib::Response& res) {
        participants_controller->getParticipants(req, res);
    });
    
    server.Post("/events/([0-9a-fA-F-]+)/participants", [participants_controller](const httplib::Request& req, httplib::Response& res) {
        participants_controller->addParticipant(req, res);
    });
    
    server.Put("/events/([0-9a-fA-F-]+)/participants/([0-9a-fA-F-]+)", [participants_controller](const httplib::Request& req, httplib::Response& res) {
        participants_controller->updateParticipant(req, res);
    });
    
    server.Delete("/events/([0-9a-fA-F-]+)/participants/([0-9a-fA-F-]+)", [participants_controller](const httplib::Request& req, httplib::Response& res) {
        participants_controller->removeParticipant(req, res);
    });

    server.Get("/events/([0-9a-fA-F-]+)/settlements", [settlements_controller](const httplib::Request& req, httplib::Response& res) {
        settlements_controller->getEventSettlements(req, res);
    });

    server.Post("/events/([0-9a-fA-F-]+)/payments", [settlements_controller](const httplib::Request& req, httplib::Response& res) {
        settlements_controller->recordPayment(req, res);
    });

    server.Get("/events/([0-9a-fA-F-]+)/settlements", [settlements_controller](const httplib::Request& req, httplib::Response& res) {
        settlements_controller->getEventSettlements(req, res);
    });

    server.Post("/events/([0-9a-fA-F-]+)/payments", [settlements_controller](const httplib::Request& req, httplib::Response& res) {
        settlements_controller->recordPayment(req, res);
    });

    server.Get("/users/balance", [settlements_controller](const httplib::Request& req, httplib::Response& res) {
        settlements_controller->getUserBalance(req, res);
    });
    
    //server.set_error_handler([](const httplib::Request&, httplib::Response& res) {
    //    json error = {
    //        {"error", "Route not found"},
    //        {"status", 404}
    //    };
    //    res.status = 404;
    //    res.set_content(error.dump(), "application/json");
    //});
    
    std::cout << "Bill Service starting on " << host << ":" << port << std::endl;
    
    if (!server.listen(host, port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}