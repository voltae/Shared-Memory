# Shared-Memory
Operation Systems - Assignment 3

## Description:
Write a C- program that broadcasts messages to a receiver using a shared memory. The shared memory should be implemented as a ring buffer of size N.
The sender should not overwrite Memory, that has not been read by the receiver, and analougos the receiver should not read any memory that has not been written by the sender. This functionality should by implemented by using **Semaphores**.
The sender process reads chars forn **stdin** and sends these through the ringbuffer to the receiver, by signalizing the ready state to the receiver. The receiver reads the chars from the ringbuffer and prints thm out to **stdout**. The receiver messages the sender the readproces and gives the memory free for the next procesure.
As soon as the sender reads **EOF** from **stdin**, it signalizes this to the ringbuffer to terminate the receiver and terminates them self.

## TODO:
- [x] Signal handling should be implemented
- [x] <del>if the parameter after -m is too high, it should output an error message.</del>
- [x] <del>EOF Handling don't work properly, tested with custom EOF, this seams not to be the problem.</del>
- [x] <del>We fail the test 8. Sender</del>
- [x] <del>we don't pass the 10 Test completely. Receiver does not terminate.</del>
- [x] <del>the 11 Test we get the error: _Error in creating read-semaphore, No such file or directory_ </del>

