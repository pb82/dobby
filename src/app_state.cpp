#include "app_state.h"

#include <Adafruit_Protomatter.h>

void DrawCircleCommand::render(Adafruit_Protomatter &matrix)
{
    matrix.drawCircle(this->x, this->y, this->r, matrix.color565(0, 255, 0));
}

AppState::AppState()
{
    this->mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(this->mutex);
}

void AppState::addCommand(std::shared_ptr<AppCommand> command)
{
    if (xSemaphoreTake(this->mutex, 0))
    {
        this->commands.push(std::move(command));
        xSemaphoreGive(this->mutex);
    }
}

std::shared_ptr<AppCommand> AppState::get()
{
    std::shared_ptr<AppCommand> result = nullptr;
    if (xSemaphoreTake(this->mutex, 0))
    {
        if (this->commands.size() > 0)
        {
            result = this->commands.front();
        }
        xSemaphoreGive(this->mutex);
    }

    return result;
}

void AppState::update(int dt)
{
    if (xSemaphoreTake(this->mutex, 0))
    {
        if (this->commands.size() > 0)
        {
            this->commands.front()->update(dt);
            if (this->commands.front()->done())
            {
                this->commands.pop();
            }
        }
        xSemaphoreGive(this->mutex);
    }
}

void AppState::render(Adafruit_Protomatter &matrix)
{
    auto front = this->get();
    if (front)
    {
        front->render(matrix);
    }
}

bool AppState::getCommand(String &ref)
{
    auto front = this->get();
    if (front)
    {
        ref.concat(front->getCommand());
        return true;
    }
    return false;
}