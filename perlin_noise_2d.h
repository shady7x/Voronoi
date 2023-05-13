#pragma once

#include <iostream>
#include <cmath>
#include <ctime>
#include <random>
#include <glm/glm.hpp>

class PerlinNoise2D {
    public:
        static void generateImage(uint32_t width, uint32_t height) {
            std::vector< uint8_t > bmp;
            bmp.emplace_back(static_cast<)

            PerlinNoise2D perlin;
            for (uint32_t i = 0; i < height; ++i) {
                for (uint32_t j = 0; j < width; ++j) {

                }
            }            
        }
        
        PerlinNoise2D(uint32_t seed = time(0)) : SEED(seed) {
            std::default_random_engine engine(SEED);
            std::uniform_int_distribution< uint32_t > distribution(0, 359);
            hash.reserve(HASH_SIZE); 
            for (size_t i = 0; i < HASH_SIZE; ++i) {   
                hash.emplace_back(distribution(engine));
            }
        }

        float noise(float x, float y, int octaves = 1, float persistence = 0.5f) {
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
            return result / max;
        }

    private:
        const uint32_t SEED;
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