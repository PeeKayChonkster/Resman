# Resman
Game resource manager
It's a header(one) only resource manager.
The main goal was to pack textures, models, etc. in one file and serve it with a game executable.
Game code will access resources through Resman by string paths, and Resman will return raw file data. That way resources are abstracted from game code, file system and everthing in between.