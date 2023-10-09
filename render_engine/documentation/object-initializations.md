# Object initialization

## Status

Accepted

## Context

Initialization of the objects is crucial. Sometimes, when the object is created not all the details are known for the creation. It leads to the 'Lazy' initialization pattern. Although it can be really useful, usually makes the code really sensitive and introduces prerequisites for running certain functionalities. This makes the code readability worth and also easier to make mistakes.

## Decision

No lazy initialization. Each object either can be created and fully functional or cannot be created.

## Consequences

- Sometimes hard to find the proper abstraction to achieve this property. It can be due to the different use cases that needs to be fulfilled.
- One knows that if there is an object then that object can be used. Makes much easier the code maintainability
- Requires more effort in error handling. Like during construction time there is only two ways to not create an object when error occurs: Exception or factory pattern.
