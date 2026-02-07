#pragma once

#include <QString>

class IDistanceProgressSink
{
public:
    virtual ~IDistanceProgressSink() = default;

    virtual void statusMessage(const QString& msg) = 0;
    virtual void progressCreate(int total, const QString& msg) = 0;
    virtual void progressUpdate(int value) = 0;
    virtual void progressKill() = 0;
};
