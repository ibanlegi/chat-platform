# for implementing a multi-user chat tool

### Licence 3 Informatique - Modules Systèmes Distribués / Systèmes d'Exploitation II
### Université de Pau et des Pays de l'Adour  
### May 2024

## Table of Contents
1. [Objective](#objective)
2. [Working Folder](#working-folder)
3. [Project Compilation](#project-compilation)
   - [Compilation of the Account Management section](#compilation-of-the-account-management-section)
   - [Compilation of the Central Part section](#compilation-of-the-central-part-section)
   - [Compilation of the Client section](#compilation-of-the-client-section)
4. [Running the application](#running-the-application)
   - [Starting the Server](#starting-the-server)
   - [Starting the Client](#starting-the-client)
6. [Using the Application](#using-the-application)
   - [Client Part](#client-part)
   - [Server Part](#server-part)
8. [Tools and Extensions Used](#tools-and-extensions-used)

---

> **Note** : The project documentation is written in French.

## Objective

This project involves developing a multi-user chat application based on a client-server architecture and operating in a distributed mode. The application enables :

- User authentication through an account management system.
- Message exchange between multiple clients connected simultaneously.
- Management of connections and disconnections, with real-time updates of the active user list.
- A command-line interface for both the client and the server.
  
The project implements various inter-process communication mechanisms, including TCP/UDP sockets, Java RMI, as well as process synchronization techniques (threads, semaphores, shared memory).

---

## Working Folder

A working folder has been created :
- [Working Folder [PDF]](./Rapport_projet.pdf)

---

## Project Compilation
Although the project's executables are provided, here are the different compilation commands for the necessary files :

### Compilation of the Account Management section
```bash
cd Gestion_comptes
javac GestionComptes.java
```

> Generate the stub for GestionComptes.class (for using Java RMI), and copy it into the Partie_centrale folder.

```bash
cd Gestion_comptes
rmic GestionComptes
cp GestionComptes_Stub.class ../Partie_centrale
```

### Compilation of the Central Part section
```bash
cd Partie_Centrale
javac ClientRMI.java
gcc -o Gestion_requete Gestion_requete -pthread -lrt
gcc -o Communication Communication.c -pthread -lrt
gcc -o Main Main.c -lrt
```

### Compilation of the Client section
```bash
cd Client
gcc -o Afficheur_chat Afficheur_chat.c
gcc -o Client_chat Client_chat.c -pthread
```

> **Note :**  
> The `-pthread` option links the program to the pthread library, enabling the use of threads.  
> The `-lrt` option links the program to the librt library, facilitating the use of time-related functionalities, particularly in the context of shared memory.

---

## Running the application

### Starting the Server
#### Account Management section
```bash
cd Gestion_Comptes
rmiregistry &
java GestionComptes
```

#### Central Part section
```bash
cd Partie_centrale
./Main <nom_serveur_gestion_comptes> <numero_port>
```
> To run the Main file, you need to specify the machine name where the `GestionComptes` process is running (`IP addr` or `hostname`) and a listening port consisting of 4 digits.

### Starting the Client
> To start a client, you need to know the name of the `Main` server (`IP addr` or `hostname`) and the port number ([specified above](#central-part-section)):
```bash
cd Client
./Client_chat <nom_serveur_main> <numero_port>
```

---

## Using the Application
### Client Part
Once connected to the system, the client initially has three options :
- *Connect*
- *Create an account*
- *Quit*

The accounts are saved in the `listeComptes.txt` file with a pseudonym and password.

Once authenticated, the user can access several features :
- *View online users*
- *Send a message*
- *Disconnect*
- *Quit*

Simultaneously, a terminal opens to display received messages. If the user chooses to disconnect, they will be redirected to the initial login menu. The program stops when the user exits the system using the provided quit feature or by pressing the `CTRL+C` combination.

### Server Part
In the terminal running Main, the `Communication` program indicates the status of the server (connection status, port number, etc.) and interprets the requests sent, displaying the type of request (connection, disconnection, user list, message, etc.), as well as the positive or negative response. This display is useful in case of system malfunctions and helps pinpoint the exact issue.

It is possible to stop the server intentionally by using the `CTRL+C` combination or by typing the `exit` command in the terminal of the Main in the central part.


---

## Tools and Extensions Used

- **[Java RMI License](https://docs.oracle.com/javase/8/docs/technotes/guides/rmi/index.html)** : It is part of the Java programming language and is subject to the licensing of the **Java Development Kit (JDK)**. The **JDK by Oracle** is distributed under the **Oracle Binary Code License (BCL)** for commercial use. However, **OpenJDK**, which is an open-source implementation of Java, is distributed under the **GNU General Public License (GPL) version 2 with the Classpath Exception**.

  
*May 2024*

