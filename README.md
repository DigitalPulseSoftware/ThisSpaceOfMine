<img src="https://github.com/DigitalPulseSoftware/ThisSpaceOfMine/blob/images/logo.png?raw=true" width="256">
<img src="https://github.com/DigitalPulseSoftware/ThisSpaceOfMine/blob/images/game-image.png?raw=true" width="1280" height="720">

# This Space Of Mine

This Space Of Mine (TSOM) is a sandbox/space exploration project where the concept is to have cubic voxel-based planets (à la Minecraft) that allow you to build your spaceship to explore other planets and eventually other systems.

This is a game project inspired by three experiences I've encountered:

1. Spacebuild (an old Garry's Mod mod) focused on building a spaceship, managing its oxygen, electricity, etc.
2. Outer Wilds: exploring a system with bizarre but interesting physical laws.
3. Oxygen Not Included: micro-management game of a space colony (including atmosphere management, liquids, etc.).

However, to avoid an overly ambitious and unachievable project, the goal is to maximize incremental progress. One of the first objectives of the game is to have a sufficiently stable multiplayer base to play together (even before adding any content) and to make it easy to get the latest client version to connect to an always-active server.

Then, features will gradually be added to gather feedback during development, somewhat like an agile approach where the product is continuously tested by anyone.

Once the project is reasonably polished, I also plan to release it on Steam to make it more accessible.

# How to play

Check the latest [release](https://github.com/DigitalPulseSoftware/ThisSpaceOfMine/releases), be sure to download the right archive for your system and to download/extract the assets too in the game folder (the game should be able to do it by itself but there were issues in the past).

Launch the game, from there you can either directly connect to the server or create an account first (without an account your player data won't be saved).

# How to contribute / test locally?

Thanks a lot for wanting to contribute, there are multiple ways to help the development of this project. 

- Contribute to the game design: [see the dedicated quackback](https://tsom-idea.digitalpulse.software)
- Contribute to the Lua code (high-level interface for entities): simply edit the Lua files in your scripts folder and restart the game (the client and the server have some support for hotreloading using the `scripts.Reload()` function).
- Contribute to the C++ code: [check this page](https://github.com/DigitalPulseSoftware/ThisSpaceOfMine/wiki/How-to-build-the-game-and-setup-local-server-(for-testing-development)) to build and setup the server/game locally.
- Contribute to produceral generation: launch the game with `--planet-editor` and edit/create your own generation scripts (in scripts/planets)
