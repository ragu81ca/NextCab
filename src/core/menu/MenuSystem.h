// MenuSystem.h - State machine for menu navigation
#pragma once

#include "MenuItem.h"
#include <Arduino.h>

class MenuSystem {
public:
    MenuSystem();
    
    // Initialize with root menu
    void begin(const MenuItem* rootItems, uint8_t rootCount);
    
    // Key input handling
    void handleKey(char key);
    
    // State queries
    bool isActive() const { return _active; }
    bool isInSubmenu() const { return _menuStack[_stackDepth].isSubmenu; }
    String getAccumulatedInput() const { return _inputBuffer; }
    int getStackDepth() const { return _stackDepth; }
    
    // Navigation
    void showMainMenu();
    void exitMenu();
    void exitToSpeed();  // Return to normal operation screen
    
    // Rendering helpers (called by OledRenderer)
    const MenuItem* getCurrentMenu() const;
    uint8_t getCurrentMenuSize() const;
    const MenuItem* getCurrentItem() const;
    bool isAccumulatingInput() const { return !_inputBuffer.isEmpty(); }
    
private:
    struct MenuStackFrame {
        const MenuItem* items;
        uint8_t itemCount;
        uint8_t selectedIndex;  // Which item was selected to get here
        bool isSubmenu;
    };
    
    static const int MAX_MENU_DEPTH = 3;
    MenuStackFrame _menuStack[MAX_MENU_DEPTH];
    int _stackDepth;
    
    bool _active;
    String _inputBuffer;
    
    // Menu item selection
    void selectItem(uint8_t index);
    void executeCurrentItem();
    
    // Stack navigation
    void pushMenu(const MenuItem* items, uint8_t count, uint8_t selectedIndex, bool isSubmenu);
    void popMenu();
};
