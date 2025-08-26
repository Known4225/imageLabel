#define TURTLE_ENABLE_TEXTURES
#define TURTLE_IMPLEMENTATION
#include "turtle.h"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h" // THANK YOU https://github.com/nothings/stb
#include <time.h>

/*
TODO:
import arbitrary image files (resize?)
include statistics (number of labels, distribution, number of images)

train model? no.
*/

/* minimum height of labels (in pixels) */
const double labelMinimumWidth  = 20;
const double labelMinimumHeight = 20;

typedef struct {
    list_t *imageNames; // name
    list_t *imageData; // width, height
    list_t *labels; // list of lists that goes class, centerX, centerY, width, height (in pixels) (yolo label format)
    list_t *labelNames; // names of labels
    char labelFilename[4096];
    double textureScaleX; // pixels to coordinates
    double textureScaleY;
    double imageX; // coordinate of center of image
    double imageY;
    int32_t imageIndex;
    int8_t leftButtonVar;
    int8_t rightButtonVar;
    int8_t newLabelButtonVar;
    int8_t deleteLabelButtonVar;
    tt_button_t *leftButton;
    tt_button_t *rightButton;
    tt_button_t *newLabelButton;
    tt_button_t *deleteLabelButton;
    tt_textbox_t *newLabelTextbox;
    int32_t newLabelTextboxLastStatus;
    tt_slider_t *labelRGB[3];
    tt_textbox_t *renameLabelTextbox;
    tt_context_t *canvasContextMenu;
    int32_t contextQueue;
    int32_t canvasContextIndex;
    int8_t keys[20];
    int8_t selecting;
    int32_t movingSelection;
    int32_t resizingSelection;
    int32_t resizingDirection;
    double selectAnchorX;
    double selectAnchorY;
    double selectEndX;
    double selectEndY;
    double selectCenterX;
    double selectCenterY;
    list_t *labelColors;
    int32_t currentLabel;
    double labelRGBValue[3];
} imageLabel_t;

imageLabel_t self;

typedef enum {
    IMAGE_KEYS_LMB = 0,
    IMAGE_KEYS_RMB = 1,
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
    strcpy(self.labelFilename, osToolsFileDialog.executableFilepath);
    strcat(self.labelFilename, "autosave/labels");
    char unixTimestamp[16];
    sprintf(unixTimestamp, "%llu", time(NULL));
    strcat(self.labelFilename, unixTimestamp);
    strcat(self.labelFilename, ".lbl"); // special .lbl file that can be opened with this application
    // FILE *fpcreate = fopen(self.labelFilename, "w");
    // fclose(fpcreate);
    self.textureScaleX = 150;
    self.textureScaleY = 150;
    self.imageX = -140;
    self.imageY = 0;
    self.imageIndex = -1;

    self.leftButtonVar = 0;
    self.rightButtonVar = 0;
    self.leftButton = buttonInit("< NULL", &self.leftButtonVar, self.imageX - self.textureScaleX, self.imageY - self.textureScaleY - 10, 10);
    self.rightButton = buttonInit("NULL >", &self.rightButtonVar, self.imageX + self.textureScaleX, self.imageY - self.textureScaleY - 10, 10);
    self.leftButton -> shape = TT_BUTTON_SHAPE_TEXT;
    self.rightButton -> shape = TT_BUTTON_SHAPE_TEXT;
    self.newLabelButtonVar = 0;
    self.newLabelButton = buttonInit("Create", &self.newLabelButtonVar, self.imageX + self.textureScaleX + 29, self.imageY + self.textureScaleY - 20, 10);
    self.newLabelTextbox = textboxInit("New Label", 32, self.imageX + self.textureScaleX + 6.7, self.imageY + self.textureScaleY, 10, 100);
    self.newLabelTextboxLastStatus = 0;
    self.labelRGBValue[0] = 255.0;
    self.labelRGBValue[1] = 255.0;
    self.labelRGBValue[2] = 255.0;
    self.labelRGB[0] = sliderInit("", &self.labelRGBValue[0], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 20, 6, 100, 0, 255, 0);
    self.labelRGB[1] = sliderInit("", &self.labelRGBValue[1], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 30, 6, 100, 0, 255, 0);
    self.labelRGB[2] = sliderInit("", &self.labelRGBValue[2], TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 40, 6, 100, 0, 255, 0);
    self.labelRGB[0] -> enabled = TT_ELEMENT_HIDE;
    self.labelRGB[1] -> enabled = TT_ELEMENT_HIDE;
    self.labelRGB[2] -> enabled = TT_ELEMENT_HIDE;
    self.renameLabelTextbox = textboxInit("Rename Label", 32, self.imageX + self.textureScaleX + 160, self.imageY + self.textureScaleY, 10, 100);
    self.renameLabelTextbox -> enabled = TT_ELEMENT_HIDE;
    self.deleteLabelButtonVar = 0;
    self.deleteLabelButton = buttonInit("Delete Label", &self.deleteLabelButtonVar, self.imageX + self.textureScaleX + 210, self.imageY + self.textureScaleY - 60, 10);
    self.deleteLabelButton -> enabled = TT_ELEMENT_HIDE;

    list_t *canvasContextOptions = list_init();
    list_append(canvasContextOptions, (unitype) "delete", 's');
    self.contextQueue = -1;
    self.canvasContextIndex = 0;
    self.canvasContextMenu = contextInit(canvasContextOptions, &self.canvasContextIndex, 0, 0, 10);
    self.canvasContextMenu -> enabled = TT_ELEMENT_HIDE;

    self.selecting = 0;
    self.movingSelection = -1;
    self.resizingSelection = -1;
    self.resizingDirection = -1;
    self.labelColors = list_init();
    /* null */
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    /* red */
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    /* green */
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    /* blue */
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    /* cyan */
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    /* magenta */
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    /* yellow */
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 255.0, 'd');
    list_append(self.labelColors, (unitype) 0.0, 'd');
    self.currentLabel = 0;
}

void textureInit(const char *filepath) {
    /* clear all image data */
    list_clear(self.imageNames);
    list_clear(self.imageData);
    list_clear(self.labels);
    list_append(self.imageNames, (unitype) "null", 's'); // for some reason I cannot put an image in the first slot of the glTexImage3D
    list_append(self.imageData, (unitype) 0, 'i');
    list_append(self.imageData, (unitype) 0, 'i');
    list_append(self.labels, (unitype) list_init(), 'r');
    /*
    Notes:
    https://stackoverflow.com/questions/75976623/how-to-use-gl-texture-2d-array-for-binding-multiple-textures-as-array
    https://stackoverflow.com/questions/72648980/opengl-sampler2d-array
    */
    list_t *files = osToolsListFiles((char *) filepath);
    int pathLen = strlen(filepath) + 256;
    char filename[pathLen];
    /* setup texture parameters */
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    unsigned int texturePower[128];
    glGenTextures(25, texturePower);
    for (int i = 0; i < files -> length / 2 + 1; i++) {
        glBindTexture(GL_TEXTURE_2D, texturePower[i]);
    }
    /* each of our images are 640 by 640 */
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 640, 640, files -> length / 2 + 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    int width;
    int height;
    int nbChannels;
    unsigned char *imgData;
    /* load all textures */
    for (int i = 0; i < files -> length / 2; i++) {
        strcpy(filename, filepath);
        strcat(filename, files -> data[i * 2].s);
        imgData = stbi_load(filename, &width, &height, &nbChannels, 0);
        if (imgData != NULL) {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, self.imageNames -> length, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, imgData);
            list_append(self.imageNames, files -> data[i * 2], 's');
            list_append(self.imageData, (unitype) width, 'i');
            list_append(self.imageData, (unitype) height, 'i');
            list_append(self.labels, (unitype) list_init(), 'r');
            stbi_image_free(imgData);
        } else {
            printf("Could not load image: %s\n", filename);
        }
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    if (self.imageNames -> length > 1) {
        self.imageIndex = 1;
    }
    list_free(files);
}

/* update autosave file */
void updateLabelFile() {
    /* check for autosave folder */
    list_t *folders = osToolsListFolders(osToolsFileDialog.executableFilepath);
    if (list_count(folders, (unitype) "autosave", 's') == 0) {
        printf("%s\n", osToolsFileDialog.executableFilepath);
        char createFolder[4096];
        strcpy(createFolder, osToolsFileDialog.executableFilepath);
        strcat(createFolder, "autosave");
        osToolsCreateFolder(createFolder);
    }
    list_clear(folders);
    FILE *labelfp = fopen(self.labelFilename, "w");
    /* header (label names and colors) */
    fprintf(labelfp, "/* header */\n");
    for (int32_t i = 1; i < self.labelNames -> length; i++) {
        fprintf(labelfp, "%s %d %d %d\n", self.labelNames -> data[i].s, (int32_t) self.labelColors -> data[i * 3].d, (int32_t) self.labelColors -> data[i * 3 + 1].d, (int32_t) self.labelColors -> data[i * 3 + 2].d);
    }
    /* data (label coordinates) */
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

void saveLabelFile(char *filename) {
    FILE *labelfp = fopen(filename, "w");
    /* header (label names and colors) */
    fprintf(labelfp, "/* header */\n");
    for (int32_t i = 1; i < self.labelNames -> length; i++) {
        fprintf(labelfp, "%s %d %d %d\n", self.labelNames -> data[i].s, (int32_t) self.labelColors -> data[i * 3].d, (int32_t) self.labelColors -> data[i * 3 + 1].d, (int32_t) self.labelColors -> data[i * 3 + 2].d);
    }
    /* data (label coordinates) */
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

/* set currently selected label and load associated values */
void setCurrentLabel(int32_t value) {
    self.currentLabel = value;
    self.labelRGBValue[0] = self.labelColors -> data[self.currentLabel * 3].d;
    self.labelRGBValue[1] = self.labelColors -> data[self.currentLabel * 3 + 1].d;
    self.labelRGBValue[2] = self.labelColors -> data[self.currentLabel * 3 + 2].d;
    strcpy(self.renameLabelTextbox -> text, self.labelNames -> data[self.currentLabel].s);
    char deleteButtonStr[128] = "Delete ";
    strcat(deleteButtonStr, self.labelNames -> data[self.currentLabel].s);
    strcpy(self.deleteLabelButton -> label, deleteButtonStr);
}

/* render loop */
void render() {
    /* crash guard */
    if (self.imageIndex < 0) {
        tt_setColor(TT_COLOR_TEXT);
        turtleTextWriteString("No dataset loaded.", self.imageX, self.imageY + 20, 15, 50);
        turtleTextWriteString("Import dataset using File -> Import Images", self.imageX, self.imageY - 5, 7, 50);
        return;
    }
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
    if (self.newLabelButtonVar || (turtleKeyPressed(GLFW_KEY_ENTER) && self.newLabelTextboxLastStatus > 0)) {
        if (strlen(self.newLabelTextbox -> text) > 0) {
            list_append(self.labelNames, (unitype) self.newLabelTextbox -> text, 's');
            if (self.labelColors -> length < self.labelNames -> length * 3) {
                list_append(self.labelColors, (unitype) 255.0, 'd');
                list_append(self.labelColors, (unitype) 255.0, 'd');
                list_append(self.labelColors, (unitype) 255.0, 'd');
            }
            self.newLabelTextbox -> text[0] = '\0';
            setCurrentLabel(self.labelNames -> length - 1);
        }
        self.newLabelButtonVar = 0;
    }
    self.newLabelTextboxLastStatus = self.newLabelTextbox -> status;
    /* render image */
    turtleTexture(self.imageIndex, self.imageX - self.textureScaleX, self.imageY - self.textureScaleY, self.imageX + self.textureScaleX, self.imageY + self.textureScaleY, 0, 255, 255, 255);
    /* render all selections */
    int32_t canvasLabelHover = -1;
    int32_t canvasLabelResize = -1;
    list_t *selections = self.labels -> data[self.imageIndex].r; // all selections for this image
    for (int32_t i = 0; i < selections -> length; i += 5) {
        turtlePenColor(self.labelColors -> data[selections -> data[i].i * 3].d, self.labelColors -> data[selections -> data[i].i * 3 + 1].d, self.labelColors -> data[selections -> data[i].i * 3 + 2].d);
        turtlePenSize(1);
        double centerX = (selections -> data[i + 1].d / self.imageData -> data[self.imageIndex * 2].i * 2 - 1) * self.textureScaleX + self.imageX;
        double centerY = (selections -> data[i + 2].d / self.imageData -> data[self.imageIndex * 2 + 1].i * 2 - 1) * self.textureScaleY + self.imageY;
        double width = selections -> data[i + 3].d / self.imageData -> data[self.imageIndex * 2].i * self.textureScaleX; // actually half of the width
        double height = selections -> data[i + 4].d / self.imageData -> data[self.imageIndex * 2 + 1].i * self.textureScaleY; // actually half of the height
        turtleGoto(centerX - width, centerY - height);
        turtlePenDown();
        turtleGoto(centerX + width, centerY - height);
        turtleGoto(centerX + width, centerY + height);
        turtleGoto(centerX - width, centerY + height);
        turtleGoto(centerX - width, centerY - height);
        turtlePenUp();
        if (turtle.mouseX >= centerX - width - 5 && turtle.mouseX <= centerX + width + 5 && turtle.mouseY >= centerY - height - 5 && turtle.mouseY <= centerY + height + 5) {
            canvasLabelHover = i;
            /* check edge */
            if (turtle.mouseY - centerY > height - 3) {
                if (turtle.mouseX - centerX > width - 3) {
                    /* top right */
                    canvasLabelResize = 1;
                    osToolsSetCursor(GLFW_DLESIZE_CURSOR);
                } else if (centerX - turtle.mouseX > width - 3) {
                    /* top left */
                    canvasLabelResize = 7;
                    osToolsSetCursor(GLFW_DRESIZE_CURSOR);
                } else {
                    /* top */
                    canvasLabelResize = 0;
                    osToolsSetCursor(GLFW_VRESIZE_CURSOR);
                }
            } else if (centerY - turtle.mouseY > height - 3) {
                if (turtle.mouseX - centerX > width - 3) {
                    /* bottom right */
                    canvasLabelResize = 3;
                    osToolsSetCursor(GLFW_DRESIZE_CURSOR);
                } else if (centerX - turtle.mouseX > width - 3) {
                    /* bottom left */
                    canvasLabelResize = 5;
                    osToolsSetCursor(GLFW_DLESIZE_CURSOR);
                } else {
                    /* bottom */
                    canvasLabelResize = 4;
                    osToolsSetCursor(GLFW_VRESIZE_CURSOR);
                }
            } else if (turtle.mouseX - centerX > width - 3) {
                /* right */
                canvasLabelResize = 2;
                osToolsSetCursor(GLFW_HRESIZE_CURSOR);
            } else if (centerX - turtle.mouseX > width - 3) {
                /* left */
                canvasLabelResize = 6;
                osToolsSetCursor(GLFW_HRESIZE_CURSOR);
            }
            if (canvasLabelResize == -1) {
                osToolsSetCursor(GLFW_MOVE_CURSOR);
            }
        }
    }
    /* render UI */
    tt_setColor(TT_COLOR_TEXT);
    turtleTextWriteUnicode((unsigned char *) self.imageNames -> data[self.imageIndex].s, self.imageX, self.imageY + self.textureScaleY + 11, 10, 50);
    int32_t labelHovering = 0;
    for (int32_t i = 1; i < self.labelNames -> length; i++) {
        double xpos = self.imageX + self.textureScaleX + 20;
        double ypos = self.imageY + self.textureScaleY - 13 * i - 28;
        if (turtle.mouseX >= xpos - 5 && turtle.mouseX <= xpos + 110 && turtle.mouseY >= ypos - 6.3 && turtle.mouseY <= ypos + 6.3) {
            turtlePenColor(self.labelColors -> data[i * 3].d * 0.8, self.labelColors -> data[i * 3 + 1].d * 0.8, self.labelColors -> data[i * 3 + 2].d * 0.8);
            turtleTextWriteUnicode((unsigned char *) self.labelNames -> data[i].s, xpos, ypos, 12, 0);
            labelHovering = i;
        } else {
            turtlePenColor(self.labelColors -> data[i * 3].d, self.labelColors -> data[i * 3 + 1].d, self.labelColors -> data[i * 3 + 2].d);
            turtleTextWriteUnicode((unsigned char *) self.labelNames -> data[i].s, xpos, ypos, 10, 0);
        }
    }
    if (self.currentLabel > 0) {
        tt_setColor(TT_COLOR_TEXT);
        turtleRectangle(self.imageX + self.textureScaleX + 210 - 80, -180, self.imageX + self.textureScaleX + 210 + 80, 180);
        turtlePenColor(self.labelColors -> data[self.currentLabel * 3].d, self.labelColors -> data[self.currentLabel * 3 + 1].d, self.labelColors -> data[self.currentLabel * 3 + 2].d);
        turtleTextWriteString(">", self.imageX + self.textureScaleX + 6.7, self.imageY + self.textureScaleY - 13 * self.currentLabel - 28, 10, 0);
        /* render label editing UI */
        self.labelRGB[0] -> enabled = TT_ELEMENT_ENABLED;
        self.labelRGB[1] -> enabled = TT_ELEMENT_ENABLED;
        self.labelRGB[2] -> enabled = TT_ELEMENT_ENABLED;
        self.renameLabelTextbox -> enabled = TT_ELEMENT_ENABLED;
        self.deleteLabelButton -> enabled = TT_ELEMENT_ENABLED;
        self.labelColors -> data[self.currentLabel * 3].d = self.labelRGBValue[0];
        self.labelColors -> data[self.currentLabel * 3 + 1].d = self.labelRGBValue[1];
        self.labelColors -> data[self.currentLabel * 3 + 2].d = self.labelRGBValue[2];
        if (self.renameLabelTextbox -> status > 0) {
            strcpy(self.labelNames -> data[self.currentLabel].s, self.renameLabelTextbox -> text);
            char deleteButtonStr[128] = "Delete ";
            strcat(deleteButtonStr, self.labelNames -> data[self.currentLabel].s);
            strcpy(self.deleteLabelButton -> label, deleteButtonStr);
        }
        if (self.deleteLabelButtonVar) {
            /* delete label */
            list_delete(self.labelNames, self.currentLabel);
            list_delete(self.labelColors, self.currentLabel * 3);
            list_delete(self.labelColors, self.currentLabel * 3);
            list_delete(self.labelColors, self.currentLabel * 3);
            setCurrentLabel(0);
            self.deleteLabelButtonVar = 0;
        }
    } else {
        self.labelRGB[0] -> enabled = TT_ELEMENT_HIDE;
        self.labelRGB[1] -> enabled = TT_ELEMENT_HIDE;
        self.labelRGB[2] -> enabled = TT_ELEMENT_HIDE;
        self.renameLabelTextbox -> enabled = TT_ELEMENT_HIDE;
        self.deleteLabelButton -> enabled = TT_ELEMENT_HIDE;
    }
    /* mouse functions */
    if (canvasLabelHover == -1) {
        if (turtle.mouseX >= self.imageX - self.textureScaleX && turtle.mouseX <= self.imageX + self.textureScaleX && turtle.mouseY >= self.imageY - self.textureScaleY && turtle.mouseY <= self.imageY + self.textureScaleY) {
            osToolsSetCursor(GLFW_CROSSHAIR_CURSOR);
        } else {
            osToolsSetCursor(GLFW_ARROW_CURSOR);
        }
    }
    if (turtleMouseDown() && self.contextQueue == -1) {
        if (self.keys[IMAGE_KEYS_LMB] == 0) {
            self.keys[IMAGE_KEYS_LMB] = 1;
            if (canvasLabelHover > -1) {
                if (canvasLabelResize == -1) {
                    /* move selection */
                    self.movingSelection = canvasLabelHover;
                    self.selectAnchorX = turtle.mouseX; // used as displacement
                    self.selectAnchorY = turtle.mouseY;
                    self.selectEndX = selections -> data[self.movingSelection + 1].d; // used as center anchor
                    self.selectEndY = selections -> data[self.movingSelection + 2].d;
                } else {
                    /* resize selection */
                    self.resizingSelection = canvasLabelHover;
                    self.resizingDirection = canvasLabelResize;
                    if (self.resizingDirection == 7 || self.resizingDirection == 0 || self.resizingDirection == 1) {
                        /* top function */
                        self.selectAnchorY = turtle.mouseY;
                        self.selectCenterY = selections -> data[self.resizingSelection + 2].d;
                        self.selectEndY = selections -> data[self.resizingSelection + 4].d;
                    }
                    if (self.resizingDirection == 3 || self.resizingDirection == 4 || self.resizingDirection == 5) {
                        /* bottom function */
                        self.selectAnchorY = turtle.mouseY;
                        self.selectCenterY = selections -> data[self.resizingSelection + 2].d;
                        self.selectEndY = selections -> data[self.resizingSelection + 4].d;
                    }
                    if (self.resizingDirection == 5 || self.resizingDirection == 6 || self.resizingDirection == 7) {
                        /* left function */
                        self.selectAnchorX = turtle.mouseX;
                        self.selectCenterX = selections -> data[self.resizingSelection + 1].d;
                        self.selectEndX = selections -> data[self.resizingSelection + 3].d;
                    }
                    if (self.resizingDirection == 1 || self.resizingDirection == 2 || self.resizingDirection == 3) {
                        /* right function */
                        self.selectAnchorX = turtle.mouseX;
                        self.selectCenterX = selections -> data[self.resizingSelection + 1].d;
                        self.selectEndX = selections -> data[self.resizingSelection + 3].d;
                    }
                }
            } else if (turtle.mouseX >= self.imageX - self.textureScaleX && turtle.mouseX <= self.imageX + self.textureScaleX && turtle.mouseY >= self.imageY - self.textureScaleY && turtle.mouseY <= self.imageY + self.textureScaleY) {
                /* begin selecting */
                self.selecting = 1;
                self.selectAnchorX = turtle.mouseX;
                self.selectAnchorY = turtle.mouseY;
            }
            if (labelHovering > 0) {
                /* switch currentLabel */
                setCurrentLabel(labelHovering);
            }
        }
    } else {
        if (self.keys[IMAGE_KEYS_LMB] == 1) {
            self.keys[IMAGE_KEYS_LMB] = 0;
            if (self.selecting) {
                /* end selection */
                double centerX = (((self.selectAnchorX + self.selectEndX) / 2 - self.imageX) / self.textureScaleX + 1) * self.imageData -> data[self.imageIndex * 2].i * 0.5;
                double centerY = (((self.selectAnchorY + self.selectEndY) / 2 - self.imageY) / self.textureScaleY + 1) * self.imageData -> data[self.imageIndex * 2 + 1].i * 0.5;
                double width = fabs(self.selectAnchorX - self.selectEndX) / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i / 2;
                double height = fabs(self.selectAnchorY - self.selectEndY) / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i / 2;
                if (width > 4 && height > 4) { // lower requirement on creating width and height just so it isn't too awkward
                    list_append(selections, (unitype) self.currentLabel, 'i');
                    list_append(selections, (unitype) centerX, 'd');
                    list_append(selections, (unitype) centerY, 'd');
                    list_append(selections, (unitype) width, 'd');
                    list_append(selections, (unitype) height, 'd');
                    updateLabelFile();
                }
                self.selecting = 0;
            } else if (self.movingSelection > -1) {
                self.movingSelection = -1;
                updateLabelFile();
            } else if (self.resizingSelection > -1) {
                self.resizingSelection = -1;
                self.resizingDirection = -1;
                updateLabelFile();
            }
        }
    }
    if (self.canvasContextMenu -> enabled == TT_ELEMENT_HIDE && self.contextQueue != -1) {
        /* results of recently hit context option are in self.canvasContextIndex */
        if (self.canvasContextIndex == 0) {
            /* delete */
            for (int32_t i = 0; i < 5; i++) {
                list_delete(selections, self.contextQueue);
                updateLabelFile();
            }
        }
        if (self.canvasContextIndex > 0) {
            /* switch label to this */
            selections -> data[self.contextQueue].i = self.canvasContextIndex;
            updateLabelFile();
        }
        self.contextQueue = -1;
    }
    if (turtleMouseRight()) {
        if (self.keys[IMAGE_KEYS_RMB] == 0) {
            self.keys[IMAGE_KEYS_RMB] = 1;
            if (canvasLabelHover > -1) {
                list_clear(self.canvasContextMenu -> options);
                list_append(self.canvasContextMenu -> options, (unitype) "delete", 's');
                for (int32_t i = 1; i < self.labelNames -> length; i++) {
                    // printf("%s\n", self.labelNames -> data[i].s);
                    list_append(self.canvasContextMenu -> options, self.labelNames -> data[i], 's');
                    // list_append(self.canvasContextMenu -> options, (unitype) "option", 's');
                }
                contextCalculateMax(self.canvasContextMenu);
                self.canvasContextMenu -> enabled = TT_ELEMENT_ENABLED;
                self.canvasContextMenu -> x = turtle.mouseX;
                self.canvasContextMenu -> y = turtle.mouseY;
                self.contextQueue = canvasLabelHover;
            }
        }
    } else {
        self.keys[IMAGE_KEYS_RMB] = 0;
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
        turtlePenColor(self.labelColors -> data[self.currentLabel * 3].d, self.labelColors -> data[self.currentLabel * 3 + 1].d, self.labelColors -> data[self.currentLabel * 3 + 2].d);
        turtlePenSize(1);
        turtleGoto(self.selectAnchorX, self.selectAnchorY);
        turtlePenDown();
        turtleGoto(self.selectEndX, self.selectAnchorY);
        turtleGoto(self.selectEndX, self.selectEndY);
        turtleGoto(self.selectAnchorX, self.selectEndY);
        turtleGoto(self.selectAnchorX, self.selectAnchorY);
        turtlePenUp();
    } else if (self.movingSelection > -1) {
        selections -> data[self.movingSelection + 1].d = self.selectEndX + ((turtle.mouseX - self.selectAnchorX) / self.textureScaleX) * self.imageData -> data[self.imageIndex * 2].i * 0.5;
        selections -> data[self.movingSelection + 2].d = self.selectEndY + ((turtle.mouseY - self.selectAnchorY) / self.textureScaleY) * self.imageData -> data[self.imageIndex * 2 + 1].i * 0.5;
        if (selections -> data[self.movingSelection + 1].d + selections -> data[self.movingSelection + 3].d / 2 > self.imageData -> data[self.imageIndex * 2].i) {
            selections -> data[self.movingSelection + 1].d = self.imageData -> data[self.imageIndex * 2].i - selections -> data[self.movingSelection + 3].d / 2;
        }
        if (selections -> data[self.movingSelection + 1].d - selections -> data[self.movingSelection + 3].d / 2 < 0) {
            selections -> data[self.movingSelection + 1].d = selections -> data[self.movingSelection + 3].d / 2;
        }
        if (selections -> data[self.movingSelection + 2].d + selections -> data[self.movingSelection + 4].d / 2 > self.imageData -> data[self.imageIndex * 2].i) {
            selections -> data[self.movingSelection + 2].d = self.imageData -> data[self.imageIndex * 2].i - selections -> data[self.movingSelection + 4].d / 2;
        }
        if (selections -> data[self.movingSelection + 2].d - selections -> data[self.movingSelection + 4].d / 2 < 0) {
            selections -> data[self.movingSelection + 2].d = selections -> data[self.movingSelection + 4].d / 2;
        }
        osToolsSetCursor(GLFW_MOVE_CURSOR);
    } else if (self.resizingSelection > -1) {
        if (self.resizingDirection == 7 || self.resizingDirection == 0 || self.resizingDirection == 1) {
            /* top function */
            selections -> data[self.resizingSelection + 4].d = self.selectEndY + (turtle.mouseY - self.selectAnchorY) / 2 / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
            selections -> data[self.resizingSelection + 2].d = self.selectCenterY + (turtle.mouseY - self.selectAnchorY) / 4 / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
            if (selections -> data[self.resizingSelection + 4].d < labelMinimumHeight) {
                selections -> data[self.resizingSelection + 4].d = labelMinimumHeight;
                selections -> data[self.resizingSelection + 2].d = self.selectCenterY - (self.selectEndY - labelMinimumHeight) / 2;
            }
            if (selections -> data[self.resizingSelection + 2].d + selections -> data[self.resizingSelection + 4].d / 2 > self.imageData -> data[self.imageIndex * 2 + 1].i) {
                double bottomBar = selections -> data[self.resizingSelection + 2].d - selections -> data[self.resizingSelection + 4].d / 2;
                selections -> data[self.resizingSelection + 4].d = self.imageData -> data[self.imageIndex * 2 + 1].i - bottomBar;
                selections -> data[self.resizingSelection + 2].d = (bottomBar + self.imageData -> data[self.imageIndex * 2 + 1].i) / 2;
            }
        }
        if (self.resizingDirection == 3 || self.resizingDirection == 4 || self.resizingDirection == 5) {
            /* bottom function */
            selections -> data[self.resizingSelection + 4].d = self.selectEndY + (self.selectAnchorY - turtle.mouseY) / 2 / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
            selections -> data[self.resizingSelection + 2].d = self.selectCenterY + (turtle.mouseY - self.selectAnchorY) / 4 / self.textureScaleY * self.imageData -> data[self.imageIndex * 2 + 1].i;
            if (selections -> data[self.resizingSelection + 4].d < labelMinimumHeight) {
                selections -> data[self.resizingSelection + 4].d = labelMinimumHeight;
                selections -> data[self.resizingSelection + 2].d = self.selectCenterY + (self.selectEndY - labelMinimumHeight) / 2;
            }
            if (selections -> data[self.resizingSelection + 2].d - selections -> data[self.resizingSelection + 4].d / 2 < 0) {
                double topBar = selections -> data[self.resizingSelection + 2].d + selections -> data[self.resizingSelection + 4].d / 2;
                selections -> data[self.resizingSelection + 4].d = topBar;
                selections -> data[self.resizingSelection + 2].d = topBar / 2;
            }
        }
        if (self.resizingDirection == 5 || self.resizingDirection == 6 || self.resizingDirection == 7) {
            /* left function */
            selections -> data[self.resizingSelection + 3].d = self.selectEndX + (self.selectAnchorX - turtle.mouseX) / 2 / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
            selections -> data[self.resizingSelection + 1].d = self.selectCenterX + (turtle.mouseX - self.selectAnchorX) / 4 / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
            if (selections -> data[self.resizingSelection + 3].d < labelMinimumWidth) {
                selections -> data[self.resizingSelection + 3].d = labelMinimumWidth;
                selections -> data[self.resizingSelection + 1].d = self.selectCenterX + (self.selectEndX - labelMinimumWidth) / 2;
            }
            if (selections -> data[self.resizingSelection + 1].d - selections -> data[self.resizingSelection + 3].d / 2 < 0) {
                double rightBar = selections -> data[self.resizingSelection + 1].d + selections -> data[self.resizingSelection + 3].d / 2;
                selections -> data[self.resizingSelection + 3].d = rightBar;
                selections -> data[self.resizingSelection + 1].d = rightBar / 2;
            }
        }
        if (self.resizingDirection == 1 || self.resizingDirection == 2 || self.resizingDirection == 3) {
            /* right function */
            selections -> data[self.resizingSelection + 3].d = self.selectEndX + (turtle.mouseX - self.selectAnchorX) / 2 / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
            selections -> data[self.resizingSelection + 1].d = self.selectCenterX + (turtle.mouseX - self.selectAnchorX) / 4 / self.textureScaleX * self.imageData -> data[self.imageIndex * 2].i;
            if (selections -> data[self.resizingSelection + 3].d < labelMinimumWidth) {
                selections -> data[self.resizingSelection + 3].d = labelMinimumWidth;
                selections -> data[self.resizingSelection + 1].d = self.selectCenterX - (self.selectEndX - labelMinimumWidth) / 2;
            }
            if (selections -> data[self.resizingSelection + 1].d + selections -> data[self.resizingSelection + 3].d / 2 > self.imageData -> data[self.imageIndex * 2].i) {
                double leftBar = selections -> data[self.resizingSelection + 1].d - selections -> data[self.resizingSelection + 3].d / 2;
                selections -> data[self.resizingSelection + 3].d = self.imageData -> data[self.imageIndex * 2].i - leftBar;
                selections -> data[self.resizingSelection + 1].d = (leftBar + self.imageData -> data[self.imageIndex * 2].i) / 2;
            }
        }
        /* cursor */
        switch (self.resizingDirection) {
        case 0:
        case 4:
            osToolsSetCursor(GLFW_VRESIZE_CURSOR);
        break;
        case 1:
        case 5:
            osToolsSetCursor(GLFW_DLESIZE_CURSOR);
        break;
        case 2:
        case 6:
            osToolsSetCursor(GLFW_HRESIZE_CURSOR);
        break;
        case 3:
        case 7:
            osToolsSetCursor(GLFW_DRESIZE_CURSOR);
        break;
        default:
            osToolsSetCursor(GLFW_ARROW_CURSOR);
        break;
        }
    }
    // list_print(selections);
}

/* removes extension from a filename, mutates the argument */
void removeExtension(char *file) {
    for (int32_t j = strlen(file) - 1; j > -1; j--) {
        if (file[j] == '.') {
            file[j] = '\0';
            break;
        }
    }
}

/* import labels from a single file (autosave file format) */
void importLabels(char *filename) {
    FILE *fp = fopen(filename, "r");
    char line[1024];
    int32_t iter = 0;
    int32_t discoveredIndex = -1;
    while (fgets(line, 1024, fp) != NULL) {
        if (iter == 0) {
            char checkHold[20] = {0};
            memcpy(checkHold, line, 12);
            if (strcmp(checkHold, "/* header */")) {
                printf("importLabels: File %s in wrong format (header not found)\n", filename);
                return;
            }
            list_clear(self.labelNames);
            list_append(self.labelNames, (unitype) "null", 's');
            iter++;
            continue;
        }
        char checkHold[20] = {0};
        memcpy(checkHold, line, 14);
        if (strcmp(checkHold, "/* labels for ") == 0) {
            /* parse image name */
            char parsedImageName[256];
            int32_t lineLength = strlen(line);
            if (lineLength > 256 + 13) {
                printf("importLabels: Line %d too long\n", iter);
            }
            memcpy(parsedImageName, line + 14, lineLength - 13);
            if (strlen(parsedImageName) < 4) {
                printf("importLabels: Parsing error on line %d\n", iter + 1);
                return;
            }
            parsedImageName[strlen(parsedImageName) - 4] = '\0';
            // printf("%s\n", parsedImageName);
            /* find parsedImageName in imageNames list */
            discoveredIndex = -1;
            for (int32_t j = 0; j < self.imageNames -> length; j++) {
                if (strcmp(self.imageNames -> data[j].s, parsedImageName) == 0) {
                    discoveredIndex = j;
                    break;
                }
            }
            if (discoveredIndex == -1) {
                printf("importLabels: Could not find image %s\n", parsedImageName);
                self.imageIndex = -1;
                return;
            }
        } else {
            if (discoveredIndex == -1) {
                /* assume in header stage */
                char labelName[48];
                int32_t red, green, blue;
                sscanf(line, "%s %d %d %d\n", labelName, &red, &green, &blue);
                list_append(self.labelNames, (unitype) labelName, 's');
                if (self.labelColors -> length < self.labelNames -> length * 3) {
                    list_append(self.labelColors, (unitype) (double) red, 'd');
                    list_append(self.labelColors, (unitype) (double) green, 'd');
                    list_append(self.labelColors, (unitype) (double) blue, 'd');
                } else {
                    self.labelColors -> data[(self.labelNames -> length - 1) * 3].d = (double) red;
                    self.labelColors -> data[(self.labelNames -> length - 1) * 3 + 1].d = (double) green;
                    self.labelColors -> data[(self.labelNames -> length - 1) * 3 + 2].d = (double) blue;
                }
            } else {
                /* assume data entry */
                int32_t class;
                double centerX, centerY, width, height;
                sscanf(line, "%d %lf %lf %lf %lf\n", &class, &centerX, &centerY, &width, &height);
                if (class >= self.labelNames -> length) {
                    printf("importLabels: Invalid class on line %d\n", iter);
                    return;
                }
                list_append(self.labels -> data[discoveredIndex].r, (unitype) class, 'i');
                list_append(self.labels -> data[discoveredIndex].r, (unitype) centerX, 'd');
                list_append(self.labels -> data[discoveredIndex].r, (unitype) centerY, 'd');
                list_append(self.labels -> data[discoveredIndex].r, (unitype) width, 'd');
                list_append(self.labels -> data[discoveredIndex].r, (unitype) height, 'd');
            }
        }
        iter++;
    }
    fclose(fp);
}

/* import labels from folder */
void importLabelsFolder(char *folderpath) {
    list_t *files = osToolsListFiles(folderpath);
    for (int32_t i = 0; i < files -> length; i += 2) {
        /* strip extension */
        char file[strlen(files -> data[i].s) + 1];
        strcpy(file, files -> data[i].s);
        removeExtension(file);
        /* search for file in image list */
        int32_t foundImage = -1;
        for (int32_t j = 1; j < self.imageNames -> length; j++) {
            char image[strlen(self.imageNames -> data[j].s) + 1];
            strcpy(image, self.imageNames -> data[j].s);
            removeExtension(image);
            if (strcmp(file, image) == 0) {
                foundImage = j;
                break;
            }
        }
        if (foundImage != -1) {
            /* populate label data */
            list_clear(self.labels -> data[foundImage].r);
            char fullname[strlen(folderpath) + strlen(files -> data[i].s) + 2];
            strcpy(fullname, folderpath);
            strcat(fullname, files -> data[i].s);
            FILE *fp = fopen(fullname, "r");
            char line[1024];
            while (fgets(line, 1024, fp) != NULL) {
                int32_t class;
                double centerX, centerY, width, height;
                sscanf(line, "%d %lf %lf %lf %lf\n", &class, &centerX, &centerY, &width, &height);
                // printf("%d %lf %lf %lf %lf\n", class, centerX, centerY, width, height);
                list_append(self.labels -> data[foundImage].r, (unitype) class, 'i');
                list_append(self.labels -> data[foundImage].r, (unitype) (centerX * self.imageData -> data[foundImage * 2].i), 'd');
                list_append(self.labels -> data[foundImage].r, (unitype) (centerY * self.imageData -> data[foundImage * 2 + 1].i), 'd');
                list_append(self.labels -> data[foundImage].r, (unitype) (width * self.imageData -> data[foundImage * 2].i), 'd');
                list_append(self.labels -> data[foundImage].r, (unitype) (height * self.imageData -> data[foundImage * 2 + 1].i), 'd');
            }
        }
    }
    list_free(files);
}

/* export labels to labels/ folder using the YOLO format */
void exportLabels(char *folderpath) {
    /* populate folder with labels */
    for (int32_t i = 1; i < self.imageNames -> length; i++) {
        /* remove file extension */
        int32_t pathLength = strlen(folderpath);
        char name[strlen(self.imageNames -> data[i].s) + pathLength + 12];
        strcpy(name, folderpath);
        strcat(name, self.imageNames -> data[i].s);
        for (int32_t j = strlen(self.imageNames -> data[i].s) + strlen(folderpath); j > pathLength; j--) {
            if (name[j] == '.') {
                name[j] = '\0';
                break;
            }
        }
        strcat(name, ".txt");
        FILE *fp = fopen(name, "w");
        list_t *selections = self.labels -> data[i].r;
        for (int32_t j = 0; j < selections -> length; j += 5) {
            fprintf(fp, "%d %lf %lf %lf %lf\n", selections -> data[j].i, selections -> data[j + 1].d / self.imageData -> data[i * 2].i, selections -> data[j + 2].d / self.imageData -> data[i * 2 + 1].i, selections -> data[j + 3].d / self.imageData -> data[i * 2].i, selections -> data[j + 4].d / self.imageData -> data[i * 2 + 1].i);
        }
        fclose(fp);
    }
}

void parseRibbonOutput() {
    if (ribbonRender.output[0] == 0) {
        return;
    }
    ribbonRender.output[0] = 0;
    if (ribbonRender.output[1] == 0) { // File
        if (ribbonRender.output[2] == 1) { // Import Images
            if (osToolsFileDialogPrompt(0, 0, 1, "", NULL) != -1) {
                textureInit(osToolsFileDialog.selectedFilenames -> data[0].s);
                printf("Imported images from %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (ribbonRender.output[2] == 2) { // Import lbl
            if (osToolsFileDialogPrompt(0, 0, 0, "", NULL) != -1) {
                importLabels(osToolsFileDialog.selectedFilenames -> data[0].s);
                printf("Imported labels from: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (ribbonRender.output[2] == 3) { // Save lbl
            if (osToolsFileDialogPrompt(1, 0, 0, "labels.lbl", NULL) != -1) {
                saveLabelFile(osToolsFileDialog.selectedFilenames -> data[0].s);
                printf("Saved labels to: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (ribbonRender.output[2] == 4) { // Import Label Folder
            if (osToolsFileDialogPrompt(0, 0, 1, "", NULL) != -1) {
                importLabelsFolder(osToolsFileDialog.selectedFilenames -> data[0].s);
                printf("Imported labels from: %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
        }
        if (ribbonRender.output[2] == 5) { // Export Label Folder
            if (osToolsFileDialogPrompt(1, 0, 1, "", NULL) != -1) {
                exportLabels(osToolsFileDialog.selectedFilenames -> data[0].s);
                printf("Exported labels to %s\n", osToolsFileDialog.selectedFilenames -> data[0].s);
            }
            /* save a backup export in the labels/ folder */
            char constructedFilepath[4096];
            strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
            strcat(constructedFilepath, "labels/");
            list_t *folders = osToolsListFolders(osToolsFileDialog.executableFilepath);
            if (list_count(folders, (unitype) "labels", 's') < 1) {
                /* create labels folder */
                osToolsCreateFolder(constructedFilepath);
            } else {
                /* delete all files in labels */
                osToolsDeleteFolder(constructedFilepath);
                osToolsCreateFolder(constructedFilepath);
            }
            list_free(folders);
            exportLabels(constructedFilepath);
            printf("Exported labels to labels/ folder\n");
        }
    }
    if (ribbonRender.output[1] == 1) { // Edit
        if (ribbonRender.output[2] == 1) { // Undo
            printf("Undo not implemented\n");
        }
        if (ribbonRender.output[2] == 2) { // Redo
            printf("Redo not implemented\n");
        }
        if (ribbonRender.output[2] == 3) { // Cut
            printf("Cut not implemented\n");
        }
        if (ribbonRender.output[2] == 4) { // Copy
            printf("Copy not implemented\n");
        }
        if (ribbonRender.output[2] == 5) { // Paste
            printf("Pasted not implemented\n");
        }
    }
    if (ribbonRender.output[1] == 2) { // View
        if (ribbonRender.output[2] == 1) { // Change theme
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
            printf("GLFW settings not implemented\n");
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
    GLFWwindow *window = glfwCreateWindow(windowHeight * 16 / 9, windowHeight, "Image Label", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, windowHeight * 16 / 9 * 0.4, windowHeight * 0.4, windowHeight * 16 / 9, windowHeight);

    /* initialise turtle */
    turtleInit(window, -320, -180, 320, 180);
    glfwSetWindowSize(window, windowHeight * 16 / 9 * 0.85, monitorSize -> height * 0.85); // doing it this way ensures the window spawns in the top left of the monitor and fixes resizing limits
    /* initialise osTools */
    osToolsInit(argv[0], window); // must include argv[0] to get executableFilepath, must include GLFW window
    osToolsFileDialogAddGlobalExtension("lbl"); // add lbl to extension restrictions
    char constructedFilepath[4096];
    /* initialise turtleText */
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/roberto.tgl");
    turtleTextInit(constructedFilepath);
    /* initialise turtleTools ribbon */
    turtleBgColor(30, 30, 30);
    turtleToolsSetTheme(TT_THEME_DARK); // dark theme preset
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/ribbonConfig.txt");
    ribbonInit(constructedFilepath);
    /* initialise turtleTools popup */
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/popupConfig.txt");
    popupInit(constructedFilepath, -60, -20, 60, 20);

    init();
    /* load default dataset under dataset/ */
    list_t *checkFolder = osToolsListFolders(osToolsFileDialog.executableFilepath);
    if (list_count(checkFolder, (unitype) "dataset", 's')) {
        strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
        strcat(constructedFilepath, "dataset/");
        textureInit(constructedFilepath);
    }
    list_free(checkFolder);
    if (argc > 1) {
        /* load labels from files */
        importLabels(argv[1]);
    } else {
        /* load default labels from labels/ folder */
        list_t *checkFolder = osToolsListFolders(osToolsFileDialog.executableFilepath);
        if (list_count(checkFolder, (unitype) "labels", 's')) {
            strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
            strcat(constructedFilepath, "labels/");
            importLabelsFolder(constructedFilepath);
        }
        list_free(checkFolder);
    }

    uint32_t tps = 120; // ticks per second (locked to fps in this case)
    uint64_t tick = 0; // count number of ticks since application started
    clock_t start, end;

    while (turtle.popupClose == 0) {
        start = clock();
        turtleGetMouseCoords();
        turtleClear();
        render();
        tt_setColor(TT_COLOR_SLIDER_BAR);
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