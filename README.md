# Nuit d'Halloween — Scène virtuelle OpenGL

Projet du cours de Programmation graphique — Polytech Marseille, Informatique 4A  
Auteure : Ahyat Guessoum

## Vidéo de démonstration

▶️ [Voir la vidéo] (compilation + visite virtuelle) : https://drive.google.com/drive/folders/1DG8vXDJFeupaSPZOo9-V1D5YJ2MCdmI7

## Description

Une nuit d'Halloween devant une maison hantée : un garçon éclaire la porte avec sa lampe torche,
des monstres sont réunis en cercle autour d'un chaudron, trois fantômes lumineux font la ronde,
deux feux de camp et des lanternes éclairent l'allée, et Saturne domine le ciel étoilé.

Notions mises en œuvre :
- chargement de maillages OBJ (VAO/VBO) et textures ;
- éclairage de Blinn-Phong : lumière directionnelle, lumières ponctuelles avec atténuation, spot light ;
- matériaux par objet (coefficient spéculaire, brillance) ;
- skybox, brouillard exponentiel (exp²), transparence par alpha test ;
- animation (fantômes en mouvement, lumières qui vacillent) ;
- caméra FPS et visite automatique (interpolation avec glm::mix et glm::lookAt).

## Compilation (Linux / WSL)

```bash
mkdir build && cd build
cmake ..
make
./MyScene
```

## Commandes

| Touche | Action |
|---|---|
| C | Lancer / arrêter la visite automatique |
| Clic gauche + souris | Tourner la caméra |
| W A S D Z X | Se déplacer |
| Molette | Zoom |
| F | Allumer / éteindre la lampe torche |
| F1 | Mode fil de fer |
| Échap | Quitter |

## Crédits

Code de base : Lighting_Multiple.cpp et classes du cours (Camera, Mesh, ShaderProgram, Texture2D).  
Bibliothèques : OpenGL 3.3, GLFW, GLEW, GLM, stb_image.

Modèles 3D (Sketchfab, licence [CC-BY 4.0](http://creativecommons.org/licenses/by/4.0/)) :
- [Graveyard Entrance](https://sketchfab.com/3d-models/graveyard-entrance-e0aa0d480159465fa26795c26546a3b6) par Reydara
- [Witches' Cauldron](https://sketchfab.com/3d-models/witches-cauldron-88559ba87aec444f8ed7636fbb58d6b2) par AK
- [Saturn (planet)](https://sketchfab.com/3d-models/saturn-planet-9ab1eb3bb97f4e4a9305c0aae2d280a6) par SebastianSosnowski
- [Ghost](https://sketchfab.com/3d-models/ghost-28b3753124a64d17ae1373853da72b28) par photon (that one larry)
- [Stylized Low-Poly Wendigo](https://sketchfab.com/3d-models/stylized-low-poly-wendigo-e93de51dcccf4525babc4249e2c43d7c) par PolyGoreStudio
- [Free Pack - Tree](https://sketchfab.com/3d-models/free-pack-tree-196525a9936a40c1ba7e74ce5c071d59) par PolyOne Studio