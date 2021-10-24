# CorePartitionOS

Version 2.7.0 release

![image](https://user-images.githubusercontent.com/1805792/125191254-6591cf80-e239-11eb-9e89-d7500e793cd4.png)

What is atomic? Atomic is a general purpose **cooperative** thread lib for embedded applications (single core or confined within other RTOS) that allows you partition your application "context" (since core execution) into several controlled context using cooperative thread. So far here nothing out of the ordinary, right? Lets think again:

* **DO NOT DISPLACE STACK, IT WILL STILL AVAILABLE FOR PROCESSING**
* Since it implements Cooperative thread every execution will atomic between contexts.
* CoreX **DO NOT DISPLACE STACK**, yes, it will use a novel technique that allow you use full stack memory freely, and once done, just call `Yield()` to switch the context.
    1. Allow you to use all your stack during thread execution and only switch once back to an appropriate place
    ``` StacK
        *-----------*
        |           | Yield()
        |           |    thread 0..N
        |           |     |       .  - After execution execution
        |           |     |      /|\   is done, the developer can
        |           |     |       |    choose wether to switch 
        |           |     |       |    context, using only what is
        |           |    \|/      |    necessary
        |           |     ---------
        |           |     - During context
        *-----------*       can goes deeper as 
                            necessary
    ```
                            
* Full prepared for IPC (_Inter Process Communication_)
    * Thread safe Queues for data communication
    * EVERY Lock can transport information a message
    * Message is composed by "size_t message" and a "size_t tag"
        * This novel concept of "tag" for a message gives the message meaning.
        
