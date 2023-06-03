#pragma once

#include <iostream>
#include <cmath>
#include <ctime>
#include <random>
#include <glm/glm.hpp>

#include "map_tile.h"

class PerlinNoise2D {
    public:
        void saveImage(uint32_t width, uint32_t height, float res = 2, int32_t octaves = 1) {
            uint32_t padSize = (4 - (width * 3) % 4) % 4;
            uint32_t fileSize = (width + padSize) * height * 3 + 54;
            //all integer little-endian
            std::vector< uint8_t > bmpHeaders = {
                // BITMAPFILEHEADER
                'B', 'M',  // BM
                static_cast< uint8_t >(fileSize), static_cast< uint8_t >(fileSize >> 8), // full file size
                static_cast< uint8_t >(fileSize >> 16), static_cast< uint8_t >(fileSize >> 24), 
                0, 0, // Reserved, app data 
                0, 0, // Reserved, app data
                54, 0, 0, 0, // starting address of the pixel array
                // BITMAPINFOHEADER
                40, 0, 0, 0, // INFOHEADER size
                static_cast< uint8_t >(width), static_cast< uint8_t >(width >> 8), // Width
                static_cast< uint8_t >(width >> 16), static_cast< uint8_t >(width >> 24),
                static_cast< uint8_t >(height), static_cast< uint8_t >(height >> 8), // Height
                static_cast< uint8_t >(height >> 16), static_cast< uint8_t >(height >> 24),
                1, 0,       // number color planes
                24, 0,      // bits per pixel
                0, 0, 0, 0, // compression is none
                0, 0, 0, 0, // This is the size of the raw bitmap data; a dummy 0 can be given for pixel matrix
                0, 0, 0, 0, // horizontal pixel per metre, 0 undefinded
                0, 0, 0, 0, // vertical pixel per metre, 0 undefinded
                0, 0, 0, 0, // the number of colors in the color palette, or 0 to default to 2^n
                0, 0, 0, 0  // the number of important colors used, or 0 when every color is important; generally ignored
            };
            std::ofstream image("perlin_noise_" + std::to_string(octaves) + ".bmp", std::ios_base::binary);
            for (const auto& b : bmpHeaders) {
                image << b;
            }
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {

                    // uint8_t color = static_cast< uint8_t >(round(255 * noiseVal));
                    // image << color << color << color;
                    // if (x < 32 && y < 24) { // окно камеры
                    //     image << static_cast< uint8_t >(0) << static_cast< uint8_t >(0) << static_cast< uint8_t >(255);
                    // } else 

                    glm::vec3 color = MapTile::getColor(MapTile::getTile(noise(x / res, y / res, octaves)));
                    image << static_cast<uint8_t>(round(255 * color.b)) << static_cast<uint8_t>(round(255 * color.g)) << static_cast<uint8_t>(round(255 * color.r));
                }
                for (uint32_t i = 0; i < padSize; ++i) {
                    image << static_cast< uint8_t >(0);
                }
            }        
            image.close();    
        }
        
        PerlinNoise2D(uint32_t seed = time(0)) : SEED(seed) {
            std::default_random_engine engine(SEED);
            std::uniform_int_distribution< uint32_t > distribution(0, 359);
            hash.reserve(HASH_SIZE); 
            for (size_t i = 0; i < HASH_SIZE; ++i) {   
                hash.emplace_back(distribution(engine));
            }
        }

        // возвращает значение шума [0, 1]
        float noise(float x, float y, int32_t octaves = 1, float persistence = 0.5f) {
            float amplitude = 1, max = 0, result = 0;
            while (octaves-- > 0) {
                max += amplitude;
                
                uint32_t left = floorf(x), top = floorf(y);
                float lx = x - left, ly = y - top;
                glm::vec2 tx1 = getPseudorandomVector(left, top);
                glm::vec2 tx2 = getPseudorandomVector(left + 1, top);
                glm::vec2 bx1 = getPseudorandomVector(left, top + 1);
                glm::vec2 bx2 = getPseudorandomVector(left + 1, top + 1);
                glm::vec2 tl(lx, ly), tr(lx - 1, ly), bl(lx, ly - 1), br(lx - 1, ly - 1);
                float tls = scalarMul(tx1, tl), trs = scalarMul(tx2, tr), bls = scalarMul(bx1, bl), brs = scalarMul(bx2, br);            
                result += lerp(lerp(tls, trs, smoothstep(lx)), lerp(bls, brs, smoothstep(lx)), smoothstep(ly)) * amplitude;
                    
                amplitude *= persistence;
                x *= 2;
                y *= 2;
            }
            return std::min(std::max(((result / max) / sqrt(2) + 0.5), 0.0), 1.0); // Нормируем от [-sqrt(2)/2, sqrt(2)/2] к [0, 1]
        }

    private:
        const uint64_t SEED;
        const size_t HASH_SIZE = 256;
        std::vector< uint32_t > hash;

        glm::vec2 getPseudorandomVector(uint32_t x, uint32_t y) {
            float rad = M_PI * hash[(x + hash[(y + SEED) % HASH_SIZE]) % HASH_SIZE] / 180;
            return glm::vec2(cos(rad), sin(rad));
        }

        float smoothstep(float t) {
            return t * t * (3 - 2 * t); 
        }

        float lerp(float a, float b, float t) {
            return a + t * (b - a);
        }

        float scalarMul(const glm::vec2& a, const glm::vec2& b) {
            return a.x * b.x + a.y * b.y;
        }
};