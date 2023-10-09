# Singletons

## Status

accepted

## Context

Each application/module has global states. There are several ways to handle these kind of globals like singletons, global variable, local variable passed around. 
Each of these techniques has some pro and cons and although there is no global good solution, here we need to made the less evil one.

What is the problem with global variables? Usually the initialization and recreation.
 - It is hard to track what was already initialized and what didn't. 
 - When and how to destroy it. 
 - Handling 'dangling' references
 - Usually makes really hard to do a proper shut down and that leads hard to achieve a restart feature. 
 - Without restart hard to make tests.
## Decision

The decision was to use a combination of global variables and passing local states. The goal is to have only one singleton. This singleton is the **Context** of the module or the application.
This context has a proper initialization and destroy function. It makes to maintainable the proper shut down process.

## Consequences

- Still have a singleton that needs to be thread-safe.
- Destroy can make a proper shutdown but still cannot check the dangling references. This is a problem that should solved in a different layer. Like using borrow pointers.
- Has only one global object. It makes nice the initialization and the way of accessing to a member makes it clear the dependencies. (less spageti code)

