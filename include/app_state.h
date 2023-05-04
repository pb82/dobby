#ifndef APP_STATE_H
#define APP_STATE_H

#include <queue>
#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <Adafruit_Protomatter.h>

struct AppCommand
{
    virtual ~AppCommand() {}

    virtual bool done() = 0;

    virtual void render(Adafruit_Protomatter &matrix){};

    virtual void update(int dt) {}

    virtual String getCommand() { return ""; }
};

struct LuaCommand : public AppCommand
{
public:
    ~LuaCommand() {}
    LuaCommand(String command) : command(command) {}

    bool done() override
    {
        return isDone;
    }

    void update(int dt) override {}

    void render(Adafruit_Protomatter &matrix) override {}

    String getCommand() override
    {
        isDone = true;
        return command;
    }

    String command;
    bool isDone = false;
};

struct DrawCircleCommand : public AppCommand
{
public:
    ~DrawCircleCommand() {}
    DrawCircleCommand(uint8_t x, uint8_t y, uint8_t r, int duration) : x(x), y(y), r(r), duration(duration) {}

    bool done() override
    {
        return timePassed >= duration;
    }

    void update(int dt) override
    {
        this->timePassed += dt;
    }

    void render(Adafruit_Protomatter &matrix) override;

private:
    uint8_t x, y, r;
    int duration;
    int timePassed = 0;
};

class AppState
{
public:
    static AppState &instance()
    {
        static AppState instance;
        return instance;
    }

    AppState(AppState const &) = delete;

    void operator=(AppState const &) = delete;

    void addCommand(std::shared_ptr<AppCommand> command);

    bool getCommand(String &ref);

    std::shared_ptr<AppCommand> get();

    void update(int dt);

    void render(Adafruit_Protomatter &matrix);

private:
    AppState();

    std::queue<std::shared_ptr<AppCommand>> commands;

    SemaphoreHandle_t mutex;
};

#endif