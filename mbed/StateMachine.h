#ifndef STATE_MACHINE
#define STATE_MACHINE

enum State{
    INIT = 0,
    SLEEPING,
    ANNOUNCING_BUTTON_PRESS,
    ANNOUNCING_BATTERY_STATUS 
};

class StateMachine {
    public:
        void setState(const State& newState);
        const State& getState();
    
    protected:
        void transitionToState(const State& newState);
        
    private:
        State state;
        
};

#endif