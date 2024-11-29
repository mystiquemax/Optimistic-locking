# Optimistic locking

The project focuses on the Hybrid Lock, which includes exclusive, shared, and optimistic states, enabling it to leverage the benefits of all these locking mechanisms. To support optimistic locking, the project also implements epoch-based reclamation. The Hybrid Lock was tested on a multithreaded data structure, and the results were evaluated in the report (see PDF file). Additionally, an optimistic lock was implemented using the PostgreSQL library in C++ to compare the results.

The report compares pessimistic and optimistic locking and evaluates their performance in various scenarios. Furthermore, it investigates real-world implementations of optimistic locking and discusses when outdated resources in the limbo bag should be deleted.

# Files in the repository

The folder *project* contains three subfolders: *include*, *postgresql*, *src* and *test*. The folder *test* tests the corresponding parts of the code. The folder *src* contains all the implemented code: the data structure, the Hybrid Lock and common function. The folder *postgresql* contains the implementation with PostgreSQL library in C++. The folder *include* contains the headers.
