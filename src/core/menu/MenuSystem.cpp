// MenuSystem.cpp
#include "MenuSystem.h"
#include "../../../WiTcontroller.h"
#include "../ThrottleManager.h"
#include "../OledRenderer.h"
#include "../../../static.h"

extern int keypadUseType;
extern OledRenderer oledRenderer;
extern ThrottleManager throttleManager;

MenuSystem::MenuSystem() : _stackDepth(0), _active(false) {}

void MenuSystem::begin(const MenuItem* rootItems, uint8_t rootCount) {
    _menuStack[0].items = rootItems;
    _menuStack[0].itemCount = rootCount;
    _menuStack[0].selectedIndex = 0;
    _menuStack[0].isSubmenu = false;
    _stackDepth = 0;
    _active = false;
    _inputBuffer = "";
}

void MenuSystem::handleKey(char key) {
    if (!_active) return;
    
    if (key == '*') {
        // Cancel - go back one level or exit
        if (_stackDepth > 0) {
            popMenu();
            oledRenderer.renderNewMenu(*this);
        } else {
            exitMenu();
        }
        return;
    }
    
    if (key == '#') {
        // Execute current selection
        executeCurrentItem();
        return;
    }
    
    if (key >= '0' && key <= '9') {
        // Map keys: '1'→index 0, '2'→index 1, ... '9'→index 8, '0'→index 9
        int index = (key == '0') ? 9 : (key - '1');
        
        // Check if we're in input mode for a TEXT_INPUT item (depth > 0 and current item is TEXT_INPUT)
        const MenuItem* current = getCurrentItem();
        if (_stackDepth > 0 && current && current->type == MenuItemType::TEXT_INPUT) {
            // We're in a TEXT_INPUT context - accumulate digits
            _inputBuffer += key;
            oledRenderer.renderNewMenu(*this);
            return;
        }
        
        // Otherwise, select the menu item at this index
        if (index >= 0 && index < getCurrentMenuSize()) {
            selectItem(index);
        }
    }
}

void MenuSystem::selectItem(uint8_t index) {
    const MenuItem* menu = getCurrentMenu();
    if (!menu || index >= getCurrentMenuSize()) return;
    
    const MenuItem& item = menu[index];
    
    switch (item.type) {
        case MenuItemType::ACTION:
            // Execute immediately
            if (item.handler) {
                MenuContext ctx;
                ctx.input = "";
                ctx.selectedIndex = index;
                ctx.currentThrottleIndex = throttleManager.getCurrentThrottleIndex();
                ctx.menuSystem = this;
                item.handler(ctx);
            }
            exitToSpeed();
            break;
            
        case MenuItemType::TEXT_INPUT:
            // Start accumulating input
            _inputBuffer = "";
            // Push single item as array with selectedIndex=0
            pushMenu(&item, 1, 0, false);
            oledRenderer.renderNewMenu(*this);
            break;
            
        case MenuItemType::LIST:
            // Show list renderer - it takes over the display
            if (item.listRenderer) {
                item.listRenderer();
                // List renderer changes keypadUseType and takes control
                // Exit menu system
                _active = false;
                _stackDepth = 0;
                _inputBuffer = "";
            }
            break;
            
        case MenuItemType::SUBMENU:
            // Push submenu onto stack
            if (item.submenuItems && item.submenuCount > 0) {
                pushMenu(item.submenuItems, item.submenuCount, index, true);
                oledRenderer.renderNewMenu(*this);
            }
            break;
    }
}

void MenuSystem::executeCurrentItem() {
    const MenuItem* current = getCurrentItem();
    if (!current) return;
    
    if (current->type == MenuItemType::TEXT_INPUT && current->handler) {
        MenuContext ctx;
        ctx.input = _inputBuffer;
        ctx.selectedIndex = 0;
        ctx.currentThrottleIndex = throttleManager.getCurrentThrottleIndex();
        ctx.menuSystem = this;
        current->handler(ctx);
        _inputBuffer = "";
        // Exit menu system
        _active = false;
        _stackDepth = 0;
        // If handler didn't change keypadUseType (e.g., didn't show roster), return to speed
        if (keypadUseType == KEYPAD_USE_OPERATION) {
            oledRenderer.renderSpeed();
        }
    }
}

void MenuSystem::showMainMenu() {
    _active = true;
    _stackDepth = 0;
    _inputBuffer = "";
    
    // Reset all stack frames to clean state
    for (int i = 0; i < MAX_MENU_DEPTH; i++) {
        _menuStack[i].selectedIndex = 0;
        if (i > 0) {
            _menuStack[i].items = nullptr;
            _menuStack[i].itemCount = 0;
            _menuStack[i].isSubmenu = false;
        }
    }
    
    keypadUseType = KEYPAD_USE_OPERATION;
    // Ensure old menu state is cleared
    extern bool menuCommandStarted;
    extern String menuCommand;
    menuCommandStarted = false;
    menuCommand = "";
    oledRenderer.renderNewMenu(*this);
}

void MenuSystem::exitMenu() {
    _active = false;
    _stackDepth = 0;
    _inputBuffer = "";
    exitToSpeed();
}

void MenuSystem::exitToSpeed() {
    _active = false;
    keypadUseType = KEYPAD_USE_OPERATION;
    oledRenderer.renderSpeed();
}

const MenuItem* MenuSystem::getCurrentMenu() const {
    if (_stackDepth < 0 || _stackDepth >= MAX_MENU_DEPTH) return nullptr;
    return _menuStack[_stackDepth].items;
}

uint8_t MenuSystem::getCurrentMenuSize() const {
    if (_stackDepth < 0 || _stackDepth >= MAX_MENU_DEPTH) return 0;
    return _menuStack[_stackDepth].itemCount;
}

const MenuItem* MenuSystem::getCurrentItem() const {
    if (_stackDepth < 0 || _stackDepth >= MAX_MENU_DEPTH) return nullptr;
    return &_menuStack[_stackDepth].items[_menuStack[_stackDepth].selectedIndex];
}

void MenuSystem::pushMenu(const MenuItem* items, uint8_t count, uint8_t selectedIndex, bool isSubmenu) {
    if (_stackDepth >= MAX_MENU_DEPTH - 1) return;
    _stackDepth++;
    _menuStack[_stackDepth].items = items;
    _menuStack[_stackDepth].itemCount = count;
    _menuStack[_stackDepth].selectedIndex = selectedIndex;
    _menuStack[_stackDepth].isSubmenu = isSubmenu;
}

void MenuSystem::popMenu() {
    if (_stackDepth > 0) {
        _stackDepth--;
        _inputBuffer = "";
    }
}
