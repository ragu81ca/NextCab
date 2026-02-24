#pragma once
#include <Arduino.h>

/// Data model for a paginated list screen.
/// Built by the handler, drawn by the Renderer — no domain knowledge crosses the boundary.
struct PagedListModel {
    static constexpr int MAX_ITEMS = 10;

    struct Item {
        String label;
        bool   invert = false;
    };

    Item   items[MAX_ITEMS];
    int    itemCount     = 0;
    int    pageCapacity  = 0;      // items-per-page (used for layout even when itemCount < capacity)
    String headerText;             // optional: displayed in row 0 before items
    String footerText;
    bool   halfPageSplit = false;

    void addItem(const String &label, bool invert = false) {
        if (itemCount < MAX_ITEMS) {
            items[itemCount].label  = label;
            items[itemCount].invert = invert;
            itemCount++;
        }
    }
};
