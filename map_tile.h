#pragma once

#include <glm/glm.hpp>

class MapTile {
    public:
        enum Type {
            WATER,
            SHORE,
            PLAIN,
            MOUNTAIN,
            HIGH_MOUNTAIN
        };

        // value значение высоты [0, 1]
        static Type getTile(float value) {
            if (value < 0.4) {
                return WATER;
            } else if (value < 0.45) {
                return SHORE;
            } else if (value > 0.7) {
                return HIGH_MOUNTAIN;
            } else if (value > 0.55) {
                return MOUNTAIN;
            } else {
                return PLAIN;
            }
        }

        static glm::vec3 getColor(Type type) {
            switch (type) {
                case 0:
                    return {0, 0.74, 1.0};
                case 1:
                    return {0.87, 0.76, 0.58};
                case 2:
                    return {0.2, 0.6, 0.2};
                case 3:
                    return {0.2549, 0.2784, 0.2901};
                case 4:
                    return {1.0, 1.0, 1.0};
            }
            return {0, 0, 0};
        }  
};