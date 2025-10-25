// MenuItem.cpp
#include "MenuItem.h"

MenuItem MenuItem::action(uint8_t id, const char* title, const char* instructions,
                         std::function<void(MenuContext&)> handler) {
    MenuItem item;
    item.id = id;
    item.title = title;
    item.instructions = instructions;
    item.type = MenuItemType::ACTION;
    item.handler = handler;
    item.listRenderer = nullptr;
    item.submenuItems = nullptr;
    item.submenuCount = 0;
    return item;
}

MenuItem MenuItem::input(uint8_t id, const char* title, const char* instructions,
                        std::function<void(MenuContext&)> handler) {
    MenuItem item;
    item.id = id;
    item.title = title;
    item.instructions = instructions;
    item.type = MenuItemType::TEXT_INPUT;
    item.handler = handler;
    item.listRenderer = nullptr;
    item.submenuItems = nullptr;
    item.submenuCount = 0;
    return item;
}

MenuItem MenuItem::list(uint8_t id, const char* title, const char* instructions,
                       std::function<void(MenuContext&)> handler,
                       std::function<void()> listRenderer) {
    MenuItem item;
    item.id = id;
    item.title = title;
    item.instructions = instructions;
    item.type = MenuItemType::LIST;
    item.handler = handler;
    item.listRenderer = listRenderer;
    item.submenuItems = nullptr;
    item.submenuCount = 0;
    return item;
}

MenuItem MenuItem::submenu(uint8_t id, const char* title, const char* instructions,
                          const MenuItem* items, uint8_t count) {
    MenuItem item;
    item.id = id;
    item.title = title;
    item.instructions = instructions;
    item.type = MenuItemType::SUBMENU;
    item.handler = nullptr;
    item.listRenderer = nullptr;
    item.submenuItems = items;
    item.submenuCount = count;
    return item;
}
