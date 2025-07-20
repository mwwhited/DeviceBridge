- **Low Priority**
  - add an option to the serial mode to run in a vt100 terminal model
    - this will allow file transfers, positional rendering, colors and much more
- **HIGH Priority**
  - create a component for the HEARTBEAT LED based on the IComponent interface.    
  - encapsulate the last time update into the IComponent interface.  
    - then cleanup and factor so a the components are just an array/vector of references to the services   │
  │ - the update function should be update to take a reference to a currentTime value.
  - while refactoring feel free to change call to use pointers by reference if it will reduce RAM allocations
  - since the ServiceLocator validates all of it's own references to components you can remove all of the child dependency validation calls
  - each component should retain its own self-test function. it should check validity, initialization and device access for owned information
  - when a component is registered it should be validated to not be a null component.
    - if a null pointer at registration or dependency validation is detected the HEARTBEAT LED should blink in ...---... {1s pause} ...---... and so on as well as there should be a loop sending a message over the serial with the name of the components that are not registered correctly every 5 second
    