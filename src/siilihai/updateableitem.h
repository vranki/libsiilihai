#ifndef UPDATEABLEITEM_H
#define UPDATEABLEITEM_H

class UpdateableItem
{
public:
    UpdateableItem();
    virtual void markToBeUpdated(bool toBe=true);
    virtual bool needsToBeUpdated() const;
private:
    bool _needsToBeUpdated;
};

#endif // UPDATEABLEITEM_H
