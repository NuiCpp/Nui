#pragma once

class Component
{
    virtual void create(/*... attributes*/) = 0;
    virtual void destroy() = 0;
};