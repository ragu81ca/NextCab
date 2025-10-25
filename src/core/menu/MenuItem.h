// MenuItem.h - Declarative menu item definition
#pragma once

#include <Arduino.h>
#include <functional>

// Menu item types determine behavior
enum class MenuItemType {
    ACTION,        // Execute immediately (e.g., Power Toggle, Direction)
    TEXT_INPUT,    // Needs additional input (e.g., Add Loco "123")
    LIST,          // Show selection list (e.g., Roster, Functions)
    SUBMENU        // Opens another menu (e.g., Extras)
};

// Forward declarations
class MenuSystem;
struct MenuContext;

// Menu item definition (data-driven)
struct MenuItem {
    uint8_t id;                          // Menu position (0-9)
    const char* title;                   // Display name
    const char* instructions;            // Bottom bar text
    MenuItemType type;                   // Behavior type
    
    // Handler function: called when item is selected
    // - For ACTION: executed immediately
    // - For INPUT: called with accumulated string when # pressed
    // - For LIST: called when list item selected
    // - For SUBMENU: switches active menu
    std::function<void(MenuContext&)> handler;
    
    // Optional renderer for LIST types (e.g., renderRoster, renderFunctionList)
    std::function<void()> listRenderer;
    
    // Optional submenu (for SUBMENU type)
    const MenuItem* submenuItems;
    uint8_t submenuCount;
    
    // Constructor helpers for different types
    static MenuItem action(uint8_t id, const char* title, const char* instructions,
                          std::function<void(MenuContext&)> handler);
    
    static MenuItem input(uint8_t id, const char* title, const char* instructions,
                         std::function<void(MenuContext&)> handler);
    
    static MenuItem list(uint8_t id, const char* title, const char* instructions,
                        std::function<void(MenuContext&)> handler,
                        std::function<void()> listRenderer);
    
    static MenuItem submenu(uint8_t id, const char* title, const char* instructions,
                           const MenuItem* items, uint8_t count);
};

// Context passed to menu handlers
struct MenuContext {
    String input;              // Accumulated input string (for INPUT type)
    int selectedIndex;         // Selected list index (for LIST type)
    int currentThrottleIndex;  // Active throttle
    MenuSystem* menuSystem;    // Back-reference for navigation
};
