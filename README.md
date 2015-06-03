# DVec-Routing
CS 118, Spring 2014
Professor: Ciaran McGoldrick

Project 2: A Simple Distance Vector Routing Protocol, written in C.

Authors:
  Jeffrey Tai, 504147859
  Brian Chang, 304151550
  Mark Matney, 504052097

I. Implementation

  A. Overview

  B. Structures

  C. Message Structure

  D. Routing/Forwarding

II. Challenges

Difficulties faced, and how we overcame.

- routers sending DV (in buffer form) to multiple sockets
- router A successfully sends to its neighbors, b sends to its neighbors, but while loop stops there
