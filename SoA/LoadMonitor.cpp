#include "stdafx.h"
#include "LoadMonitor.h"

LoadMonitor::LoadMonitor() :
m_lock(),
m_completionCondition() {
    m_completedTasks.store(0);
}
LoadMonitor::~LoadMonitor() {
    if (m_internalTasks.size() > 0) {
        for (ILoadTask* t : m_internalTasks) {
            if (t) delete t;
        }
    }
    for (auto& t : m_internalThreads) {
        t.detach();
    }
}

void LoadMonitor::addTask(const nString& name, ILoadTask* task) {
    m_tasks.emplace(name, task);
}
bool LoadMonitor::isTaskFinished(const nString& task) {
    m_lock.lock();
    bool state = isFinished(task);
    m_lock.unlock();
    return state;
}

bool LoadMonitor::isFinished(const nString& task) {
    auto& kvp = m_tasks.find(task);
    if (kvp == m_tasks.end()) {
        fprintf(stderr, "LoadMonitor Warning: dependency %s does not exist\n", task.c_str());
        return false;
    }
    return kvp->second->isFinished();
}
bool LoadMonitor::canStart(const nString& task) {
    // Check that the dependency exists
    auto& kvp = m_tasks.find(task);
    if (kvp == m_tasks.end()) {
        fprintf(stderr, "LoadMonitor Warning: task %s does not exist\n", task.c_str());
        return false;
    }
    // Check all dependencies
    for (auto& dep : kvp->second->dependencies) {
        if (!isFinished(dep)) {
            return false;
        }
    }
    return true;
}

void LoadMonitor::start() {
    LoadMonitor* monitor = this;
    for (auto& kvp : m_tasks) {
        ILoadTask* task = kvp.second;
        const nString& name = kvp.first;
        m_internalThreads.emplace_back([=] () {
            // Wait For Dependencies To Complete
            std::unique_lock<std::mutex> uLock(monitor->m_lock);
            m_completionCondition.wait(uLock, [=] {
#ifdef DEBUG
                printf("CHECK: %s\r\n", name.c_str());
#endif
                return monitor->canStart(name);
            });
            uLock.unlock();

#ifdef DEBUG
            printf("BEGIN: %s\r\n", name.c_str());
            task->doWork();
            printf("END: %s\r\n", name.c_str());
#else
            task->doWork();
#endif // DEBUG

            // Notify That This Task Is Completed
            uLock.lock();
            m_completedTasks++;
            m_completionCondition.notify_all();
            uLock.unlock();
        });
    }
}
void LoadMonitor::wait() {
    // Wait for all threads to complete
    for (auto& t : m_internalThreads) {
        t.join();
    }

    m_internalThreads.clear();

    // Free all tasks
    for (ILoadTask* t : m_internalTasks) delete t;
    m_internalTasks.clear();
}

void LoadMonitor::setDep(const nString& name, const nString& dep) {
    // Check that the task exists
    auto& kvp = m_tasks.find(name);
    if (kvp == m_tasks.end()) {
        fprintf(stderr, "LoadMonitor Warning: Task %s doesn't exist.\n",name.c_str());
        return;
    }

    // Check that the dependency exists
    auto& dvp = m_tasks.find(dep);
    if (dvp == m_tasks.end()) {
        fprintf(stderr, "LoadMonitor Warning: Dependency %s doesn't exist.\n", dep.c_str());
        return;
    }

    // Add the dependency
    kvp->second->dependencies.insert(dep);
}


