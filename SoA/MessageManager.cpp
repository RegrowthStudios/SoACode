#include "stdafx.h"
#include "MessageManager.h"


MessageManager::MessageManager()
{
}


MessageManager::~MessageManager()
{
}


void MessageManager::enqueue(ThreadId thread, Message message) {
    if (thread == ThreadId::UPDATE) {
        physicsToRender.enqueue(message);
    } else { // ThreadName::RENDERING
        renderToPhysics.enqueue(message);
    }
}

bool MessageManager::tryDeque(ThreadId thread, Message& result) {
    if (thread == ThreadId::UPDATE) {
        return renderToPhysics.try_dequeue(result);
    } else { // ThreadName::RENDERING
        return physicsToRender.try_dequeue(result);
    }
}

void MessageManager::waitForMessage(ThreadId thread, MessageID idToWait, Message& result) {
    if (thread == ThreadId::UPDATE) {
        while (true){
            if (renderToPhysics.try_dequeue(result)){
                if (result.id == idToWait || result.id == MessageID::QUIT){
                    return;
                }
            }
        }
    } else { // ThreadName::RENDERING
        while (true){
            if (physicsToRender.try_dequeue(result)){
                if (result.id == idToWait || result.id == MessageID::QUIT){
                    return;
                }
            }
        }
    }

}