
#ifndef __CREATOR_H_
#define __CREATOR_H_

class CreatorThread : public Thread {
    void entry( void );
    int pcus;
public:
    CreatorThread( int _pcus )
        : Thread( "creator", 10, 16384 ) { pcus = _pcus; resume( tid ); }
    ~CreatorThread( void ) { }
};

#endif /* __CREATOR_H_ */
