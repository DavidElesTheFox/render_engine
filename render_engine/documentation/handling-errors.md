# Handling errors

## Status

Prototype 

## Context

The old question: Using exceptions or not? 

In C++ real time applications usually no exceptions are used, it is because its cost. 

Exceptions when they occurs has a higher cost due to stack rewind operation. It is costly only when the exception is occurred.
On the other hand using exceptions gives the nice separation of the happy and unhappy path in the code. One is not disturb  with error handling code when it is not necessary.

## Decision

Because it is an experimental engine let's use exceptions. The key is what is an exception. Exception is used to handle real errors, when something went wrong. So, it is not about control path handling, like when an object is looked for and not found it is not the time when exception is required. But, when a GPU is not there but it should be there that's an exception - something that we didn't expect.

## Consequences

- When an exception happens frequently it will cause huge slow down in the application. Big performance hit.
- When exceptions are used properly no performance hit is expected. 
- Increased application stability - no crash when error occurred.
- Increased code separation and readability.
