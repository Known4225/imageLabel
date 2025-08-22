#define TURTLE_ENABLE_TEXTURES
#define TURTLE_IMPLEMENTATION
#include "turtle.h"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h" // THANK YOU https://github.com/nothings/stb
#include <time.h>

/*
TODO:
make labelColors a list not an array
rename labels (textbox that preloads label name when currentLabel changes and directly corresponds to the label name)
delete label (button)
resize and delete label (on canvas)
export dataset

train model? no.
*/

typedef struct {
    list_t *imageNames; // name
    list_t *imageData; // width, height
    list_t *labels; // list of lists that goes class, centerX, centerY, width, height (in pixels) (yolo label format)
    list_t *labelNames;
    char labelFilename[256];
    double textureScaleX;
    double textureScaleY;
    double imageX;
    double imageY;
    int32_t imageIndex;
    int8_t leftButtonVar;
    int8_t rightButtonVar;
    int8_t newLabelButtonVar;
    tt_button_t *leftButton;
    tt_button_t *rightButton;
    tt_button_t *newLabelButton;
    tt_textbox_t *newLabelTextbox;
    tt_slider_t *labelRGB[3];
    tt_textbox_t *renameLabelTextbox;
    int8_t keys[20];
    int8_t selecting;
    double selectAnchorX;
    double selectAnchorY;
    double selectEndX;
    double selectEndY;
    double labelColors[120];
    int32_t currentLabel;
    double labelRGBValue[3];
} imageLabel_t;

imageLabel_t self;

typedef enum {
    IMAGE_KEYS_LMB = 0,
    IMAGE_KEYS_LEFT = 3,
    IMAGE_KEYS_RIGHT = 4,
} keyIndex_t;

void init() {
    self.imageNames = list_init();
    self.imageData = list_init();
    self.labels = list_init();
    self.labelNames = list_init();
    list_append(self.imageNames, (unitype) "null", 's'); // for some reason I cannot put an image in the first slot of the glTexImage3D
    list_append(self.imageData, (unitype) 0, 'i');
    list_append(self.imageData, (unitype) 0, 'i');
    list_append(self.labels, (unitype) list_init(), 'r');
    list_append(self.labelNames, (unitype) "null", 's');
    list_append(self.labelNames, (unitype) "label1", 's');
    list_append(self.labelNames, (unitype) "label2", 's');
    list_append(self.labelNames, (unitype) "label3", 's');
    list_append(self.labelNames, (unitype) "label4", 's');
    strcpy(self.labelFilename, "labelsAutosave/labels");
    char unixTimestamp[16];
    sprintf(unixTimestamp, "%llu", time(NULL));
    strcat(self.labelFilename, unixTimestamp);
    strcat(self.labelFilename, ".txt");
    // FILE *fpcreate = fopen(self.labelFilename, "w");
    // fclose(fpcreate);
    self.textureScaleX = 150;
    self.textureScaleY = 150;
    self.imageX = -140;
    self.imageY = 0;
    self.imageIndex = 1;

    self.leftButtonVar = 0;
    self.rightButtonVar = 0;
    self.leftButton = buttonInit("< NULL", &self.leftButtonVar, self.imageX - self.textureScaleX, self.imageY - self.textureScaleY - 10, 10);
    self.rightButton = buttonInit("NULL >", &self.rightButtonVar, self.imageX + self.textureScaleX, self.imageY - self.textureScaleY - 10, 10);
    self.leftButton -> shape = TT_BUTTON_SHAPE_TEXT;
    self.rightButton -> shape = TT_BUTTON_SHAPE_TEXT;
    self.newLabelButtonVar = 0;
    self.newLabelButton = buttonInit("Create", &self.newLabelButtonVar, self.imageX + self.textureScaleX + 29, self.imageY + self.textureScaleY - 20, 10);
    self.newLabelTextbox = textboxInit("New Label", 32, self.imageX + self.textureScaleX + 6.7, self.imageY + self.textureScaleY, 10, 100);
    self.labelRGBValue[0] = 255.0;
    self.labelRGBValue[1] = 255.0;
    self.labelRGBValue[2] = 255.0;
    self.labelRGB[0] = sliderInit("", &self.labelRGBValue[0], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 20, 6, 100, 0, 255, 0);
    self.labelRGB[1] = sliderInit("", &self.labelRGBValue[1], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 30, 6, 100, 0, 255, 0);
    self.labelRGB[2] = sliderInit("", &self.labelRGBValue[2], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 40, 6, 100, 0, 255, 0);
    self.labelRGB[0] -> enabled = TT_ELEMENT_HIDE;
    self.labelRGB[1] -> enabled = TT_ELEMENT_HIDE;
    self.labelRGB[2] -> enabled = TT_ELEMENT_HIDE;
    self.renameLabelTextbox = self.newLabelTextbox = textboxInit("Rename Label", 32, self.imageX + self.textureScaleX + 160, self.imageY + self.textureScaleY, 10, 100);
    self.renameLabelTextbox -> enabled = TT_ELEMENT_HIDE;

    self.selecting = 0;
    double labelColorsCopy[] = {
        255, 255, 255,
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        0, 255, 255,
        255, 0, 255,
        255, 255, 0,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
        255, 255, 255,
    };
    memcpy(self.labelColors, labelColorsCopy, sizeof(labelColorsCopy));
    self.currentLabel = 0;
}

void textureInit(const char *filepath) {
    /* 
    Notes:
    https://stackoverflow.com/questions/75976623/how-to-use-gl-texture-2d-array-for-binding-multiple-textures-as-array
    https://stackoverflow.com/questions/72648980/opengl-sampler2d-array
    */
    int pathLen = strlen(filepath) + 32;
    char filename[pathLen];
    /* setup texture parameters */
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    unsigned int texturePower[128];
    glGenTextures(25, texturePower);
    for (int i = 0; i < 128; i++) {
        glBindTexture(GL_TEXTURE_2D, texturePower[i]);
    }
    /* each of our images are 640 by 640 */
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 640, 640, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    int width;
    int height;
    int nbChannels;
    unsigned char *imgData;
    /* load all textures */
    for (int i = 0; i < 105; i++) {
        strcpy(filename, filepath);
        char exactName[32];
        sprintf(exactName, "image%d.jpg", i);
        strcat(filename, exactName);
        imgData = stbi_load(filename, &width, &height, &nbChannels, 0);
        if (imgData != NULL) {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, self.imageNames -> length, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, imgData);
            list_append(self.imageNames, (unitype) exactName, 's');
            list_append(self.imageData, (unitype) width, 'i');
            list_append(self.imageData, (unitype) height, 'i');
            list_append(self.labels, (unitype) list_init(), 'r');
        } else {
            printf("Could not load image: %s\n", filename);
        }
        stbi_image_free(imgData);
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void updateLabelFile() {
    FILE *labelfp = fopen(self.labelFilename, "w");
    for (int32_t i = 1; i < self.labels -> length; i++) {
        list_t *selections = self.labels -> data[i].r;
        if (selections -> length > 0) {
            fprintf(labelfp, "/* labels for %s */\n", self.imageNames -> data[i].s);
        }
        for (int32_t j = 0; j < selections -> length; j += 5) {
            fprintf(labelfp, "%d %d %d %d %d\n", selections -> data[j].i, (int32_t) selections -> data[j + 1].d, (int32_t) selections -> data[j + 2].d, (int32_t) selections -> data[j + 3].d, (int32_t) selections -> data[j + 4].d);
        }
    }
    fclose(labelfp);
}

void render() {
    /* change button position and label */
    int32_t previousImageIndex = self.imageIndex - 1;
    int32_t nextImageIndex = self.imageIndex + 1;
    if (previousImageIndex < 1) {
        previousImageIndex = self.imageNames -> length - 1;
    }
    if (nextImageIndex > self.imageNames -> length - 1) {
        nextImageIndex = 1;
    }
    strcpy(self.leftButton -> label, "< ");
    strcat(self.leftButton -> label, self.imageNames -> data[previousImageIndex].s);
    strcpy(self.rightButton -> label, self.imageNames -> data[nextImageIndex].s);
    strcat(self.rightButton -> label, " >");
    self.leftButton -> x = self.imageX - self.textureScaleX + turtleTextGetUnicodeLength((unsigned char *) self.leftButton -> label, 9) / 2;
    self.rightButton -> x = self.imageX + self.textureScaleX - turtleTextGetUnicodeLength((unsigned char *) self.rightButton -> label, 9) / 2;
    /* button and arrow key functionality */
    if (turtleKeyPressed(GLFW_KEY_LEFT) || self.leftButtonVar) {
        if (self.keys[IMAGE_KEYS_LEFT] == 0) {
            self.keys[IMAGE_KEYS_LEFT] = 70;
            self.imageIndex = previousImageIndex;
        } else if (self.keys[IMAGE_KEYS_LEFT] == 1) {
            self.keys[IMAGE_KEYS_LEFT] = 4;
            self.imageIndex = previousImageIndex;
        } else {
            self.keys[IMAGE_KEYS_LEFT]--;
        }
    } else {
        self.keys[IMAGE_KEYS_LEFT] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_RIGHT) || self.rightButtonVar) {
        if (self.keys[IMAGE_KEYS_RIGHT] == 0) {
            self.keys[IMAGE_KEYS_RIGHT] = 70;
            self.imageIndex = nextImageIndex;
        } else if (self.keys[IMAGE_KEYS_RIGHT] == 1) {
            self.keys[IMAGE_KEYS_RIGHT] = 4;
            self.imageIndex = nextImageIndex;
        } else {
            self.keys[IMAGE_KEYS_RIGHT]--;
        }
    } else {
        self.keys[IMAGE_KEYS_RIGHT] = 0;
    }
    if (self.newLabelButtonVar) {
        if (strlen(self.newLabelTextbox -> text) > 0) {
            list_append(self.labelNames, (unitype) self.newLabelTextbox -> text, 's');
            self.newLabelTextbox -> text[0] = '\0';
            self.currentLabel = self.labelNames -> length - 1;
            self.labelRGBValue[0] = self.labelColors[self.currentLabel * 3];
            self.labelRGBValue[1] = self.labelColors[self.currentLabel * 3 + 1];
            self.labelRGBValue[2] = self.labelColors[self.currentLabel * 3 + 2];
            strcpy(self.renameLabelTextbox -> text, self.labelNames -> data[self.currentLabel].s);
        }
        self.newLabelButtonVar = 0;
    }
    /* render image */
    turtleTexture(self.imageIndex, self.imageX - self.textureScaleX, self.imageY - self.textureScaleY, self.imageX + self.textureScaleX, self.imageY + self.textureScaleY, 0, 255, 255, 255);
    /* render all selections */
    list_t *selections = self.labels -> data[self.imageIndex].r; // all selections for this image
    for (int32_t i = 0; i < selections -> length; i += 5) {
        turtlePenColor(self.labelColors[selections -> data[i].i * 3], self.labelColors[selections -> data[i].i * 3 + 1], self.labelColors[selections -> data[i].i * 3 + 2]);
        turtlePenSize(2);
        double centerX = selections -> data[i + 1].d / self.imageData -> data[self.imageIndex * 2].i * self.textureScaleX + self.imageX;
        double centerY = selections -> data[i + 2].d / self.imageData -> data[self.imageIndex * 2 + 1].i * self.textureScaleY + self.imageY;
        double width = selections -> data[i + 3].d / self.imageData -> data[self.imageIndex * 2].i * self.textureScaleX;
        double height = selections -> data[i + 4].d / self.imageData -> data[self.imageIndex * 2 + 1].i * self.textureScaleY;
        turtleGoto(centerX - width / 2, centerY - height / 2);
        turtlePenDown();
        turtleGoto(centerX + width / 2, centerY - height / 2);
        turtleGoto(centerX + width / 2, centerY + height / 2);
        turtleGoto(centerX - width / 2, centerY + height / 2);
        turtleGoto(centerX - width / 2, centerY - height / 2);
        turtlePenUp();
    }
    /* render UI */
    tt_setColor(TT_COLOR_TEXT);
    turtleTextWriteUnicode((unsigned char *) self.imageNames -> data[self.imageIndex].s, self.imageX, self.imageY + self.textureScaleY + 11, 10, 50);
    int32_t labelHovering = 0;
    for (int32_t i = 1; i < self.labelNames -> length; i++) {
        double xpos = self.imageX + self.textureScaleX + 20;
        double ypos = self.imageY + self.textureScaleY - 13 * i - 28;
        if (turtle.mouseX >= xpos - 5 && turtle.mouseX <= xpos + 50 && turtle.mouseY >= ypos - 6.3 && turtle.mouseY <= ypos + 6.3) {
            turtlePenColor(self.labelColors[i * 3] * 0.8, self.labelColors[i * 3 + 1] * 0.8, self.labelColors[i * 3 + 2] * 0.8);
            turtleTextWriteUnicode((unsigned char *) self.labelNames -> data[i].s, xpos, ypos, 12, 0);
            labelHovering = i;
        } else {
            turtlePenColor(self.labelColors[i * 3], self.labelColors[i * 3 + 1], self.labelColors[i * 3 + 2]);
            turtleTextWriteUnicode((unsigned char *) self.labelNames -> data[i].s, xpos, ypos, 10, 0);
        }
    }
    if (self.currentLabel > 0) {
        turtlePenColor(self.labelColors[self.currentLabel * 3], self.labelColors[self.currentLabel * 3 + 1], self.labelColors[self.currentLabel * 3 + 2]);
        turtleTextWriteString(">", self.imageX + self.textureScaleX + 6.7, self.imageY + self.textureScaleY - 13 * self.currentLabel - 28, 10, 0);
        /* render label editing UI */
        self.labelRGB[0] -> enabled = TT_ELEMENT_ENABLED;
        self.labelRGB[1] -> enabled = TT_ELEMENT_ENABLED;
        self.labelRGB[2] -> enabled = TT_ELEMENT_ENABLED;
        self.renameLabelTextbox -> enabled = TT_ELEMENT_ENABLED;
        self.labelColors[self.currentLabel * 3] = self.labelRGBValue[0];
        self.labelColors[self.currentLabel * 3 + 1] = self.labelRGBValue[1];
        self.labelColors[self.currentLabel * 3 + 2] = self.labelRGBValue[2];
        strcpy(self.labelNames -> data[self.currentLabel].s, self.renameLabelTextbox -> text);
    }
    /* mouse functions */
    if (turtle.mouseX >= self.imageX - self.textureScaleX && turtle.mouseX <= self.imageX + self.textureScaleX && turtle.mouseY >= self.imageY - self.textureScaleY && turtle.mouseY <= self.imageY + self.textureScaleY) {
        osToolsSetCursor(GLFW_CROSSHAIR_CURSOR);
    } else {
        osToolsSetCursor(GLFW_ARROW_CURSOR);
    }
    if (turtleMouseDown()) {
        if (self.keys[IMAGE_KEYS_LMB] == 0) {
            self.keys[IMAGE_KEYS_LMB] = 1;
            if (turtle.mouseX >= self.imageX - self.textureScaleX && turtle.mouseX <= self.imageX + self.textureScaleX && turtle.mouseY >= self.imageY - self.textureScaleY && turtle.mouseY <= self.imageY + self.textureScaleY) {
                /* begin selecting */
                self.selecting = 1;
                self.selectAnchorX = turtle.mouseX;
                self.selectAnchorY = turtle.mouseY;
            }
            if (labelHovering > 0) {
                /* switch currentLabel */
                self.currentLabel = labelHovering;
                self.labelRGBValue[0] = self.labelColors[self.currentLabel * 3];
                self.labelRGBValue[1] = self.labelColors[self.currentLabel * 3 + 1];
                self.labelRGBValue[2] = self.labelColors[self.currentLabel * 3 + 2];
                strcpy(self.renameLabelTextbox -> text, self.labelNames -> data[self.currentLabel].s);
            }
        }
    } else {
        if (self.keys[IMAGE_KEYS_LMB] == 1) {
            self.keys[IMAGE_KEYS_LMB] = 0;
            if (self.selecting) {
                /* end selection */
                double centerX = ((self.selectAnchorX + self.selectEndX) / 2 - self.imageX) / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
                double centerY = ((self.selectAnchorY + self.selectEndY) / 2 - self.imageY) / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
                double width = fabs(self.selectAnchorX - self.selectEndX) / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
                double height = fabs(self.selectAnchorY - self.selectEndY) / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
                list_append(self.labels -> data[self.imageIndex].r, (unitype) self.currentLabel, 'i');
                list_append(self.labels -> data[self.imageIndex].r, (unitype) centerX, 'd');
                list_append(self.labels -> data[self.imageIndex].r, (unitype) centerY, 'd');
                list_append(self.labels -> data[self.imageIndex].r, (unitype) width, 'd');
                list_append(self.labels -> data[self.imageIndex].r, (unitype) height, 'd');
                updateLabelFile();
                self.selecting = 0;
            }
        }
    }
    if (self.selecting) {
        if (turtle.mouseX >= self.imageX - self.textureScaleX && turtle.mouseX <= self.imageX + self.textureScaleX) {
            self.selectEndX = turtle.mouseX;
        } else {
            if (turtle.mouseX < self.imageX - self.textureScaleX) {
                self.selectEndX = self.imageX - self.textureScaleX;
            } else {
                self.selectEndX = self.imageX + self.textureScaleX;
            }
        }
        if (turtle.mouseY >= self.imageY - self.textureScaleY && turtle.mouseY <= self.imageY + self.textureScaleY) {
            self.selectEndY = turtle.mouseY;
        } else {
            if (turtle.mouseY < self.imageY - self.textureScaleY) {
                self.selectEndY = self.imageY - self.textureScaleY;
            } else {
                self.selectEndY = self.imageY + self.textureScaleY;
            }
        }
        turtlePenColor(self.labelColors[self.currentLabel * 3], self.labelColors[self.currentLabel * 3 + 1], self.labelColors[self.currentLabel * 3 + 2]);
        turtlePenSize(2);
        turtleGoto(self.selectAnchorX, self.selectAnchorY);
        turtlePenDown();
        turtleGoto(self.selectEndX, self.selectAnchorY);
        turtleGoto(self.selectEndX, self.selectEndY);
        turtleGoto(self.selectAnchorX, self.selectEndY);
        turtleGoto(self.selectAnchorX, self.selectAnchorY);
        turtlePenUp();
    }
}

void parseRibbonOutput() {
    if (ribbonRender.output[0] == 0) {
        return;
    }
    ribbonRender.output[0] = 0;
    if (ribbonRender.output[1] == 0) { // File
        if (ribbonRender.output[2] == 1) { // New
            printf("New\n");
        }
        if (ribbonRender.output[2] == 2) { // Save
            if (strcmp(osToolsFileDialog.selectedFilename, "null") == 0) {
                if (osToolsFileDialogPrompt(1, "") != -1) {
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                }
            } else {
                printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
            }
        }
        if (ribbonRender.output[2] == 3) { // Save As...
            if (osToolsFileDialogPrompt(1, "") != -1) {
                printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
            }
        }
        if (ribbonRender.output[2] == 4) { // Open
            if (osToolsFileDialogPrompt(0, "") != -1) {
                printf("Loaded data from: %s\n", osToolsFileDialog.selectedFilename);
            }
        }
    }
    if (ribbonRender.output[1] == 1) { // Edit
        if (ribbonRender.output[2] == 1) { // Undo
            printf("Undo\n");
        }
        if (ribbonRender.output[2] == 2) { // Redo
            printf("Redo\n");
        }
        if (ribbonRender.output[2] == 3) { // Cut
            osToolsClipboardSetText("test123");
            printf("Cut \"test123\" to clipboard!\n");
        }
        if (ribbonRender.output[2] == 4) { // Copy
            osToolsClipboardSetText("test345");
            printf("Copied \"test345\" to clipboard!\n");
        }
        if (ribbonRender.output[2] == 5) { // Paste
            osToolsClipboardGetText();
            printf("Pasted \"%s\" from clipboard!\n", osToolsClipboard.text);
        }
    }
    if (ribbonRender.output[1] == 2) { // View
        if (ribbonRender.output[2] == 1) { // Change theme
            printf("Change theme\n");
            if (tt_theme == TT_THEME_DARK) {
                turtleBgColor(36, 30, 32);
                turtleToolsSetTheme(TT_THEME_COLT);
            } else if (tt_theme == TT_THEME_COLT) {
                turtleBgColor(212, 201, 190);
                turtleToolsSetTheme(TT_THEME_NAVY);
            } else if (tt_theme == TT_THEME_NAVY) {
                turtleBgColor(255, 255, 255);
                turtleToolsSetTheme(TT_THEME_LIGHT);
            } else if (tt_theme == TT_THEME_LIGHT) {
                turtleBgColor(30, 30, 30);
                turtleToolsSetTheme(TT_THEME_DARK);
            }
        } 
        if (ribbonRender.output[2] == 2) { // GLFW
            printf("GLFW settings\n");
        } 
    }
}

void parsePopupOutput(GLFWwindow *window) {
    if (popup.output[0] == 0) {
        return;
    }
    popup.output[0] = 0; // untoggle
    if (popup.output[1] == 0) { // cancel
        turtle.close = 0;
        glfwSetWindowShouldClose(window, 0);
    }
    if (popup.output[1] == 1) { // close
        turtle.popupClose = 1;
    }
}

int main(int argc, char *argv[]) {
    /* Initialise glfw */
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (Anti-Aliasing) with 4 samples (must be done before window is created (?))

    /* Create a windowed mode window and its OpenGL context */
    const GLFWvidmode *monitorSize = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int32_t windowHeight = monitorSize -> height;
    GLFWwindow *window = glfwCreateWindow(windowHeight * 16 / 9, windowHeight, "turtle demo", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, windowHeight * 16 / 9 * 0.4, windowHeight * 0.4, windowHeight * 16 / 9, windowHeight);

    /* initialise turtle */
    turtleInit(window, -320, -180, 320, 180);
    glfwSetWindowSize(window, windowHeight * 16 / 9 * 0.85, monitorSize -> height * 0.85); // doing it this way ensures the window spawns in the top left of the monitor and fixes resizing limits
    /* initialise turtleText */
    turtleTextInit("config/roberto.tgl");
    /* initialise turtleTools ribbon */
    turtleBgColor(30, 30, 30);
    turtleToolsSetTheme(TT_THEME_DARK); // dark theme preset
    ribbonInit("config/ribbonConfig.txt");
    /* initialise turtleTools popup */
    popupInit("config/popupConfig.txt", -70, -20, 70, 20);
    /* initialise osTools */
    osToolsInit(argv[0], window); // must include argv[0] to get executableFilepath, must include GLFW window
    osToolsFileDialogAddExtension("txt"); // add txt to extension restrictions
    osToolsFileDialogAddExtension("png"); // add png to extension restrictions
    osToolsFileDialogAddExtension("jpeg"); // add jpeg to extension restrictions
    osToolsFileDialogAddExtension("jpg"); // add jpg to extension restrictions
    osToolsFileDialogAddExtension("bmp"); // add bmp to extension restrictions
    
    init();
    textureInit("dataset/");

    uint32_t tps = 120; // ticks per second (locked to fps in this case)
    uint64_t tick = 0; // count number of ticks since application started
    clock_t start, end;

    while (turtle.popupClose == 0) {
        start = clock();
        turtleGetMouseCoords();
        turtleClear();
        render();
        tt_setColor(TT_COLOR_TEXT);
        if (self.labelRGB[0] -> enabled != TT_ELEMENT_HIDE) {
            turtleTextWriteString("Red", self.labelRGB[0] -> x - self.labelRGB[0] -> length / 2 - self.labelRGB[0] -> size, self.labelRGB[0] -> y, self.labelRGB[0] -> size - 1, 100);
            turtleTextWriteStringf(self.labelRGB[0] -> x + self.labelRGB[0] -> length / 2 + self.labelRGB[0] -> size, self.labelRGB[0] -> y, 4, 0, "%d", (int32_t) round(self.labelRGBValue[0]));
            turtleTextWriteString("Green", self.labelRGB[1] -> x - self.labelRGB[1] -> length / 2 - self.labelRGB[1] -> size, self.labelRGB[1] -> y, self.labelRGB[1] -> size - 1, 100);
            turtleTextWriteStringf(self.labelRGB[1] -> x + self.labelRGB[1] -> length / 2 + self.labelRGB[1] -> size, self.labelRGB[1] -> y, 4, 0, "%d", (int32_t) round(self.labelRGBValue[1]));
            turtleTextWriteString("Blue", self.labelRGB[2] -> x - self.labelRGB[2] -> length / 2 - self.labelRGB[2] -> size, self.labelRGB[2] -> y, self.labelRGB[2] -> size - 1, 100);
            turtleTextWriteStringf(self.labelRGB[2] -> x + self.labelRGB[2] -> length / 2 + self.labelRGB[2] -> size, self.labelRGB[2] -> y, 4, 0, "%d", (int32_t) round(self.labelRGBValue[2]));
        }
        turtleToolsUpdate(); // update turtleTools
        parseRibbonOutput(); // user defined function to use ribbon
        parsePopupOutput(window); // user defined function to use popup
        turtleUpdate(); // update the screen
        end = clock();
        while ((double) (end - start) / CLOCKS_PER_SEC < (1.0 / tps)) {
            end = clock();
        }
        tick++;
    }
    turtleFree();
    glfwTerminate();
    return 0;
}