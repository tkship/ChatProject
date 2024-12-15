#include "IoServicePool.h"

#include <iostream>

IoServicePool& IoServicePool::GetInstance()
{
    static IoServicePool self;
    return self;
}

boost::asio::io_service& IoServicePool::GetIoService()
{
    mIndex = mIndex < mPoolSize ? mIndex : 0;

    return mServices[mIndex++];
}

IoServicePool::IoServicePool(int aPoolSize)
    : mPoolSize(aPoolSize)
    , mIndex(0)
    , mServices(aPoolSize)
{
    for (int i = 0; i < mPoolSize; ++i)
    {
        mWorks.emplace_back(std::make_unique<Work>(mServices[i]));
        mThreads.emplace_back([this, i]() {
            mServices[i].run();
        });
    }
}

IoServicePool::~IoServicePool()
{
    Stop();
    
    std::cout << "~IoServicePool" << std::endl;
}

void IoServicePool::Stop()
{
    for (int i = 0; i < mPoolSize; ++i)
    {
        mWorks[i].reset();
        mServices[i].stop();
    }

    for (int i = 0; i < mPoolSize; ++i)
    {
        mThreads[i].join();
    }
}
