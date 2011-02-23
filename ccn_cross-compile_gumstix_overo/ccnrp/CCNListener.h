#ifndef _LISTENER_H
#define _LISTENER_H

#define CCNRP_DEFAULT_PORT 9099

class Listener
{
  private:
    int m_iPort;

  public:
    Listener(int p_iPort = CCNRP_DEFAULT_PORT);
    virtual ~Listener();

    void listen();
};

#endif
