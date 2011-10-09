#include "updateableitem.h"

UpdateableItem::UpdateableItem()
{
    _needsToBeUpdated = false;
}

void UpdateableItem::markToBeUpdated(bool toBe) {
    _needsToBeUpdated = toBe;
}

bool UpdateableItem::needsToBeUpdated() const {
    return _needsToBeUpdated;
}
