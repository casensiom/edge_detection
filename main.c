#include "canny.h"
#include "hough.h"
#include "image.h"
#include "bmp.h"
#include "timer.h"
#include <stdio.h>
#include <time.h>
#include <math.h>

void draw2(SImage *_pImg, float _theta, float _rho);
void draw(SImage *_pImg, float _theta, float _rho);

int
main(int argc, char *argv[])
{
    const char *fileIn = "in.bmp";

    const uint32 maxLines = 16;
    SPolar lineBuffer[maxLines];

    SImage imgRGB, imgGray, img;

    reset_image(&imgRGB);
    reset_image(&imgGray);
    reset_image(&img);

    ELoadError error = load_image(fileIn, &imgRGB);
    if(error == LE_NO_ERROR)
    {
        imgGray = convertToGray(imgRGB);
#if 1
        copy_image(&imgGray, &img);
        canny(img.mpData, img.mWidth, img.mHeight, 50, 125);
        save_image("out.bmp", &img);
        uint32 foundLines = hough(img.mpData, img.mWidth, img.mHeight, 14, lineBuffer, maxLines);
        printf("%d lines found.\n", foundLines);


        release_image(&imgRGB);
        imgRGB = convertToARGB(img);
        uint32 i;
        for(i =0; i < foundLines; ++i)
        {
            draw(&imgRGB, lineBuffer[i].theta, lineBuffer[i].rho);
            draw2(&imgRGB, lineBuffer[i].theta, lineBuffer[i].rho);
        }
        draw_line(&imgRGB, 0xFF0000, 0, 0, imgRGB.mWidth, imgRGB.mHeight);
        save_image("out_hough.bmp", &imgRGB);
#else
        uint64 currentTime;
        uint64 lastTime;
        uint64 startTime = current_time_ms();
        uint32 frames = 0;
        uint64 cannyTime = 0;
        uint64 houghTime = 0;

        lastTime = (current_time_ms() - startTime)/1000;
        while(1)
        {
            copy_image(&imgGray, &img);

            uint64 preTime = current_time_ms();
            canny(img.mpData, img.mWidth, img.mHeight, 50, 125);
            uint64 postTime = current_time_ms();
            uint32 foundLines = hough(img.mpData, img.mWidth, img.mHeight, 8, lineBuffer, maxLines);
            uint64 postTime2 = current_time_ms();
            ++frames;
            cannyTime += postTime - preTime;
            houghTime += postTime2 - postTime;

            //printf("canny: %llums, hough: %llums, total: %llums\n", postTime - preTime, postTime2 - postTime, postTime2 - preTime);
            currentTime = (current_time_ms() - startTime)/1000;
            if(currentTime != lastTime)
            {
                printf("fps: %d, canny: %fms, hough: %fms\n", frames, (double)cannyTime / (double)frames, (double)houghTime / (double)frames);
                lastTime = currentTime;
                cannyTime = 0;
                houghTime = 0;
                frames = 0;
            }
        }
#endif
        // printf("found %d lines. (max %d)\n", foundLines, maxLines);
        // for(uint32 i = 0; i < foundLines; ++i)
        // {
        //     printf("line %d) r: %f, t: %f\n", i, lineBuffer[i].rho, lineBuffer[i].theta);
        // }

        //save_image("out.bmp", &img);

    }
    else
    {
        char *pError = "Unknonwn error";
        switch(error)
        {
            case LE_NO_ERROR:
            pError = "NO_ERROR";
            break;
            case LE_CANT_OPEN:
            pError = "CANT_OPEN";
            break;
            case LE_CANT_READ:
            pError = "CANT_READ";
            break;
            case LE_CANT_WRITE:
            pError = "CANT_WRITE";
            break;
            case LE_INVALID_MAGIC:
            pError = "INVALID_MAGIC";
            break;
        }
        printf("Error '%s' while opening '%s'\n", pError, fileIn);
    }

    release_image(&imgRGB);
    release_image(&imgGray);
    release_image(&img);
    release_canny();
    release_hough();

    return 0;
}


void draw2(SImage *_pImg, float _theta, float _rho)
{
    int x1, y1, x2, y2;
    x1 = y1 = x2 = y2 = 0;
    uint32 w = _pImg->mWidth;
    uint32 h = _pImg->mHeight;

    if(_theta >= 45*DEG2RAD && _theta <= 135*DEG2RAD)
    {
        //y = (r - x cos(t)) / sin(t)
        x1 = 0;
        y1 = ((double)(_rho) - ((x1 - (w/2) ) * cos(_theta))) / sin(_theta) + (h / 2);
        x2 = w;
        y2 = ((double)(_rho) - ((x2 - (w/2) ) * cos(_theta))) / sin(_theta) + (h / 2);
    }
    else
    {
        //x = (r - y sin(t)) / cos(t);
        y1 = 0;
        x1 = ((double)(_rho) - ((y1 - (h/2) ) * sin(_theta))) / cos(_theta) + (w / 2);
        y2 = h;
        x2 = ((double)(_rho) - ((y2 - (h/2) ) * sin(_theta))) / cos(_theta) + (w / 2);
    }
    printf("draw line2: t:%f, r:%f => (%d, %d) - (%d, %d)\n", _theta, _rho, x1, y1, x2, y2);
}


void draw(SImage *_pImg, float _theta, float _rho)
{
    int x1, y1, x2, y2;
    uint32 w = _pImg->mWidth;
    uint32 h = _pImg->mHeight;


    double      a = cos(_theta);
    double      b = sin(_theta);
    double      x0 = a * _rho;
    double      y0 = b * _rho;
    double      x = x0 - b;
    double      y = y0 + a;

    double      coefA = ((x != x0) ? ((y - y0) / (x - x0)) : 0.0);
    double      coefB = y0 - (coefA * x0);

    if (fabs(b) < 0.5)
    {//vertical line : from y = 0 to y = frameSize.height
        y1 = 0;
        x1 = coefA != 0.0 ? (int)(-coefB / coefA) : (int)x0;
        y2 = h;
        x2 = coefA != 0.0 ? (int)((y2 - coefB) / coefA) : (int)x0;
    }
    else
    {//horizontal line : from x = 0 to x = frameSize.width
        x1 = 0;
        y1 = (int)coefB;
        x2 = w;
        y2 = (int)(coefA * x2 + coefB);
    }

    printf("draw line: t:%f, r:%f => (%d, %d) - (%d, %d)\n", _theta, _rho, x1, y1, x2, y2);
    draw_line(_pImg, 0xFF00FF, x1, y1, x2, y2);
}

