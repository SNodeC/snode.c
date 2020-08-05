#ifndef MANAGEDDESCRIPTOR_H
#define MANAGEDDESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class ObservationCounter {
protected:
    int observationCounter = 0;
};

class EventReceiver : virtual public ObservationCounter {
public:
    EventReceiver() = default;

    EventReceiver(const EventReceiver&) = delete;

    EventReceiver& operator=(const EventReceiver&) = delete;

    virtual ~EventReceiver() = default;

    void observe() {
        observationCounter++;
        enabled = true;
    }

    void unobserve() {
        observationCounter--;
        enabled = false;
    }

    void checkObserved() {
        if (observationCounter == 0) {
            unobserved();
        }
    }

    bool isEnabled() const {
        return enabled;
    }

protected:
    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual void unobserved() = 0;

private:
    bool enabled = false;
};

#endif // MANAGEDDESCRIPTOR_H
