# Custom Zigbee Light Link Lights

This project contains the sources for creating custom ZLL firmware that works on any JN5168 chip, inclusing a [Mesh Bee](http://wiki.seeedstudio.com/wiki/Mesh_Bee).

Two type of lights are supported:
- Dimmable RGB light 
- Dimmable monochrome light

For details on the project please visit my blog: [PeeVeeOne.com](http://peeveeone.com/?tag=light-link)

## Steps

To edit and build the firmware please folow these steps:

- Install the NXP toolchain (JN516x) (details [here](http://peeveeone.com/?p=144)) and make sure the demo project builds
- Create a new workspace
- Import this project from Git, make sure you pick the root of the workspace as the repository location
- Select the light you want to build from the build configurations
- Build the project
- Load the firmware (details [here](http://peeveeone.com/?p=187))
- Enjoy your light

