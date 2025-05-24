
This is an implementation of the classic snake game in C, using SDL2.

![](assets/cnake_game_preview.png)

*Disclaimer*: This was developed fully in a Linux environment. Do not expect it to work on other platforms.

# Building
Make sure you have [SDL2](https://wiki.libsdl.org/SDL2/FrontPage) installed and can run the command `sdl2-config`.

Go to the project root directory and run:
```sh
$ make
```

You should now have the executable `cnake_game`.
```sh
$ ./cnake_game
```

Have fun!

# Controls
- `w` `a` `s` `d`/`arrow keys` to move the snake
- `space` to pause if the game is running, or restart if the game is over

