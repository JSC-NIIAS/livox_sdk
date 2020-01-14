#ifndef LIVOXTHREAD_H
#define LIVOXTHREAD_H

#include <QThread>

#include "customscatter.h"
#include "lds_lidar.h"

class LivoxThread : public QThread
{
    Q_OBJECT

public:

    CustomScatter *_scatter = nullptr;

    explicit LivoxThread( CustomScatter* scatter,
                          const std::vector<std::string>& broadcast );

    void run() override;

private:

    std::vector<std::string> _broadcast;
};

#endif // LIVOXTHREAD_H
