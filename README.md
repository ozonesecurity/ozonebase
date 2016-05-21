# Ozone - state of the art framework for developing security and surveillance solutions for the Home, Commercial and more.

Ozone is the next generation of ZoneMinder.
Completely re-written and more modern in architecture. While [Version 1](https://github.com/zoneminder) is still used by many, we felt its time to take a fresh approach as a lot of the code is legacy and hard to extend, not to mention that over the last 10 years, technology has evolved significantly.
Ozone comprises of:

* serverbase - base framework for developing security and surveilllance servers (like ZoneMinder, for example)
* clientbase - base framework for developing mobile apps for remote control (like zmNinja, for example)
* examples - reference implementations to get  you started


## What is the goal?

Ozone a more modern take and has been re-architected to serve as a UI-less framework at its core. We expect this to be the core framework for developing full fledged apps on top. Our hope is that this will form the base of many security solutions - developers can use this library to accelerate their own vision of security software. Over time, we will also offer our own fully running UI enabled version running over NodeJS/Angular.

## Intended audience

We expect OEMs, ISVs and 3rd party developers to use this library to build their own solutons powered by us. It is not an end consumer product, unlike V1.

As of today, there is no roadmap to merge this with version 1. Version 1 has a seperate goal from Version 2 and we think its best to leave it that way for now.

## Who is developing and maintaining Ozone?
The Ozonebase server framework is developed by [Philip Coombes](https://github.com/web2wire), the original developer of ZoneMinder.
The codebase will be maintained by Phil and [Pliable Pixels](https://github.com/pliablepixels) and other key contributors who will be added soon.

Ozonebase part of a solution suite being developed by [Ozone Networks](http://ozone.network). The full solution suite will eventually include a server framework (this code), a mobile framework for white-labelled apps and a reference solution using them.

Ozonebase is dual-licensed.
Please refer to the [LICENSING](LICENSE.md) file for details.
