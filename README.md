<div align="center">

<img width="512" height="192" alt="image" src="https://github.com/user-attachments/assets/d1b2bdd2-7c00-435d-b7a2-46b80f2f5441" />

# XMD Recompiled

### X-Men: Destiny — Native PC Port

**An ongoing reverse-engineering, recompilation and preservation project bringing the 2011 action RPG X-Men: Destiny to modern PCs.**

<br>

![Status](https://img.shields.io/badge/Status-Early%20Development-orange)
![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![Original Platform](https://img.shields.io/badge/Original-Xbox%20360%20%7C%20PS3-lightgrey)
![License](https://img.shields.io/badge/License-See%20Repository-green)

</div>

---

## 📖 What is XMD Recompiled?

**XMD Recompiled** is a community-driven effort to bring **X-Men: Destiny** to modern Windows PCs through reverse engineering and native recompilation.

This is **not an emulator**.

The goal is to study the original Xbox 360 version, understand its executable and runtime architecture, and progressively reconstruct the systems required to run the game as a native PC application.

The long-term objective is to create a version of X-Men: Destiny that can run on modern PC hardware without requiring the original console.

> **Preserve the game. Understand its technology. Bring it to modern PCs.**

---

## 🚧 Project Status

> ### 🟢 Playable — Active Development
>
> **XMD Recompiled is currently playable from start to finish, with some minor issues and glitches still being addressed.**
>
> Development is now focused on improving compatibility, polishing the experience, and implementing missing PC-specific features.

| Area | Status |
|---|---|
| Reverse Engineering | 🟡 Ongoing |
| Executable Analysis | 🟡 Ongoing |
| Game Architecture Documentation | 🟡 Ongoing |
| Static Recompilation | 🟢 Functional |
| Native Runtime | 🟢 Functional |
| Graphics | 🟢 Functional — Minor Issues |
| Audio | 🟢 Functional |
| Input | 🟡 Controller Support Functional |
| Keyboard & Mouse Support | 🟠 In Development |
| DualShock / DualSense Support | 🟡 Functional — Native Glyphs Pending |
| Xbox Controller Support | 🟢 Functional |
| Ultrawide Support | 🟠 Planned |
| Gameplay Systems | 🟢 Functional |
| Full Game Compatibility | 🟢 Playable — Minor Issues |

### Current Known Limitations

The game is already playable, but several features are still being improved or implemented:

- 🐛 Minor gameplay and graphical glitches remain
- ⌨️ Keyboard and mouse support is not yet implemented
- 🎮 DualShock and DualSense controllers work, but the game currently displays the original Xbox 360 button glyphs
- 🖥️ Ultrawide resolutions are not yet supported
- 🔧 Additional compatibility and stability improvements are ongoing

> The current focus is on turning the existing playable port into a polished, fully featured native PC experience.

### Game Architecture

Research and document systems such as:

- Application startup
- Game initialization
- Memory management
- Rendering
- Input
- Audio
- Entities and actors
- Animation
- Combat
- Abilities and powers
- X-Genes
- Save data
- Mission and level systems

### Networking

Research the game's networking architecture, including:

- Packet framing
- Opcodes
- Message dispatch
- Client/server communication
- Sessions
- Networked entities

### Native PC Runtime

The eventual goal is to replace or recreate the Xbox 360-specific runtime environment with a modern PC-compatible implementation.

This includes investigating the systems necessary to support:

- Native x86-64 execution
- Modern Windows APIs
- Keyboard and mouse input
- Controller support
- Modern GPU APIs
- Modern display resolutions
- Widescreen and ultrawide support
- Improved compatibility with current hardware

---

## 🧬 About X-Men: Destiny

Released in **2011** and developed by **Silicon Knights**, X-Men: Destiny is an action RPG set in the Marvel universe.

Unlike most X-Men games, players do not control an established hero as the main protagonist. Instead, they choose one of several original characters and develop their mutant abilities throughout the game.

### Choose Your Origin

Play as one of three unique protagonists, each with their own background and story.

### Manifest Your Powers

Develop your character using different mutant power paths and abilities.

### Choose Your Allegiance

Make choices that influence your relationship with the **X-Men** and the **Brotherhood**.

### Equip X-Genes

Collect genetic enhancements inspired by established Marvel mutants and use them to customize your combat abilities.

---

## 🛡️ Why Does This Project Exist?

X-Men: Destiny never received an official PC release.

The game was also removed from digital storefronts, leaving the original console versions as the primary way to experience it.

Games such as this are at risk of becoming increasingly difficult to access as their original hardware ages.

**XMD Recompiled exists as a preservation and technical research project.**

The goal is not only to make the game playable on modern systems, but also to document and preserve knowledge about its technology and architecture.

> Games should not disappear simply because the hardware they were designed for becomes obsolete.

---

## 🖥️ Project Vision

The long-term vision for XMD Recompiled is:

- 🎮 Native PC execution
- 🖥️ Modern resolutions
- 🖥️ Widescreen support
- 🖥️ Ultrawide support
- ⌨️ Keyboard and mouse support
- 🎮 Modern controller support
- ⚡ Improved performance
- 🔧 Compatibility improvements
- 🛠️ Modding possibilities
- 📚 Preserved technical documentation

The ultimate goal is to create the **definitive modern PC version of X-Men: Destiny** while documenting the technology required to make that possible.

---

## 🧪 Technical Approach

XMD Recompiled is being developed through a combination of:

- Reverse engineering
- Binary analysis
- Static recompilation research
- Executable analysis
- Assembly analysis
- Ghidra
- Runtime reconstruction
- Native code development
- Compatibility-layer development

The exact implementation will evolve as more of the original game's architecture is understood.

---

## 📂 Repository Structure

```text
.
├── assets/
│   └── images/          # README and project images
│
├── docs/                # Technical documentation
│
├── research/            # Reverse-engineering notes and research
│
├── src/                 # Native runtime and project source code
│
├── tools/               # Analysis and development tools
│
├── README.md
└── LICENSE
