#pragma once
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>

// Interface For A Loading Task
class ILoadTask {
public:
    ILoadTask() {
        m_isFinished.store(false);
    }

    bool isFinished() const {
        return m_isFinished.load();
    }

    virtual void load() = 0;
    void doWork() {
        load();
        m_isFinished.store(true);
    }

    std::unordered_set<nString> dependencies;
private:
    // Loading State
    std::atomic_bool m_isFinished;
};

// Loading Task Closure Wrapper
template<typename F>
class LoadFunctor : public ILoadTask {
public:
    LoadFunctor() {
        // Empty
    }
    LoadFunctor(F& f) 
        : _f(f) {
        // Empty
    }

    void load() {
        // Call The Closure
        _f();
    }
private:
    F _f;
};

// Make Use Of Compiler Type Inference For Anonymous Type Closure Wrapper
template<typename F>
inline ILoadTask* makeLoader(F f) {
    return new LoadFunctor<F>(f);
}

class LoadMonitor {
public:
    LoadMonitor();
    ~LoadMonitor();

    // Add A Task To This List (No Copy Or Delete Will Be Performed On Task Pointer)
    void addTask(const nString& name, ILoadTask* task);
    template<typename F>
    void addTaskF(const nString& name, F taskF) {
        ILoadTask* t = makeLoader(taskF);
        m_internalTasks.push_back(t);
        addTask(name, t);
    }
    
    // Make A Loading Task Dependant On Another (Blocks Until Dependency Completes)
    void setDep(const nString& name, const nString& dep);

    // Fires Up Every Task In A Separate Thread
    void start();
    // Blocks On Current Thread Until All Child Threads Have Completed
    void wait();

    // Checks If A Task Is Finished
    bool isTaskFinished(const nString& task);
    // Checks if the load monitor is finished
    bool isFinished() { return m_completedTasks.load() == (int)m_tasks.size(); }

private:
    // Is A Task Finished (False If Task Does Not Exist
    bool isFinished(const nString& task);
    // Check Dependencies Of A Task To See If It Can Begin
    bool canStart(const nString& task);

    // Tasks Mapped By Name
    std::unordered_map<nString, ILoadTask*> m_tasks;
   
    // Thread For Each Task
    std::vector<std::thread> m_internalThreads;

    // Functor Wrapper Tasks That Must Be Deallocated By This Monitor
    std::vector<ILoadTask*> m_internalTasks;

    // Monitor Lock
    std::atomic_int m_completedTasks;
    std::mutex m_lock;
    std::condition_variable m_completionCondition;
};