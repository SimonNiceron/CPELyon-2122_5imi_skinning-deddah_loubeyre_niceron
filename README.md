[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-f059dc9a6f8d3a56e377f745f24479a46679e63a5d9fe6f495e02850cd0d8118.svg)](https://classroom.github.com/online_ide?assignment_repo_id=6371009&assignment_repo_type=AssignmentRepo)
# 2122_5imi_skinning
TP noté d'animation par skinning

## Compilation

Pour compiler et executer à partir du CMakeLists.txt

```sh
cd project
mkdir build
cd build
cmake ..
make
cd ..
./build/pgm
```

ou 

```sh
cd project
mkdir build
cd build
cmake ..
cd ..
make -C ./build && ./build/pgm
```

**Note sur l'utilisation des IDE (QtCreator, etc)**

Le repertoire d'execution doit être dans project/
C'est a dire que le repertoire data/ doit être accessible.