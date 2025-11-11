#version 450 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (rgba8, binding = 0) uniform writeonly image2D destTex;

layout (std140, binding = 1) uniform Camera {
    mat4 invProjection;
    mat4 invView;
    vec3 cameraPos;
};

const ivec3 worldSize = ivec3(64, 64, 64);
const int MAX_RAYS = 8;
const vec4 skyColor = vec4(0.5, 0.7, 1.0, 1.0);

uint get_red_rgba(uint color) { return (color >> 24) & 0xffu; }
uint get_green_rgba(uint color) { return (color >> 16) & 0xffu; }
uint get_blue_rgba(uint color) { return (color >> 8) & 0xffu; }
uint get_alpha_rgba(uint color) { return color & 0xffu; }

vec4 rgbaColor(uint color) {
    return vec4(
        float(get_red_rgba(color)) / 255.0,
        float(get_green_rgba(color)) / 255.0,
        float(get_blue_rgba(color)) / 255.0,
        float(get_alpha_rgba(color)) / 255.0
    );
}

struct Voxel {
    float refraction;
    float emmission;
    float mettalicity;
};

struct VoxelObj {
    ivec3 coord;
    uint color;
    Voxel voxel;
    float padding;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    float IOF;
    float weight;
    bool defined;
    vec4 colorTint;
    float distanceInMedium;
    vec4 mediumColor;
    float mediumDensity;
};

layout (std430, binding = 2) buffer VoxelBuffer {
    VoxelObj voxels[];
};

bool isInsideWorld(ivec3 c) {
    return all(greaterThanEqual(c, ivec3(0))) && all(lessThan(c, worldSize));
}

int getVoxelIndex(ivec3 coord) {
    if(!isInsideWorld(coord)) return -1;
    return coord.x + coord.y * worldSize.x + coord.z * worldSize.x * worldSize.y;
}

bool popRay(inout Ray rays[MAX_RAYS], inout int stackSize) {
    if (stackSize <= 0) return false;
    stackSize -= 1;
    rays[stackSize].defined = false;
    return true;
}

bool addRay(inout Ray rays[MAX_RAYS], Ray ray, inout int stackSize) {
    if (!ray.defined) return false;
    if (stackSize < 0 || stackSize >= MAX_RAYS) {
        return false;
    }
    rays[stackSize] = ray;
    stackSize += 1;
    return true;
}

vec4 getVoxelColor(ivec3 coord) {
    int idx = getVoxelIndex(coord);
    if (idx < 0) return vec4(0.0);
    return rgbaColor(voxels[idx].color);
}

float sampleDensity(ivec3 c) {
    vec4 ccol = getVoxelColor(c);
    return ccol.a;
}

vec4 accumulateColors(Ray rays[MAX_RAYS]) {
    vec3 result = vec3(0.0);
    float totalWeight = 0.0;
    
    for (int i = 0; i < MAX_RAYS; ++i) {
        if (!rays[i].defined) continue;
        result += rays[i].colorTint.rgb * rays[i].weight;
        totalWeight += rays[i].weight;
    }

    if(totalWeight < 1.0){
        result /= totalWeight;
    }

    return vec4(result, 1.0);
}

bool hitMarching(in Ray thisRay, out ivec3 hitMapPos, out ivec3 prevMapPos, out vec3 hitPoint, out vec3 hitNormal) {
    const float EPS = 1e-6;
    vec3 origin = thisRay.origin;
    vec3 dir = normalize(thisRay.direction);

    ivec3 mapPos = ivec3(floor(origin));
    prevMapPos = mapPos;

    ivec3 stepSign = ivec3(sign(dir));

    vec3 tDelta = vec3(
        abs(dir.x) > EPS ? abs(1.0 / dir.x) : 1e30,
        abs(dir.y) > EPS ? abs(1.0 / dir.y) : 1e30,
        abs(dir.z) > EPS ? abs(1.0 / dir.z) : 1e30
    );

    vec3 voxelBoundary = floor(origin) + vec3(step(0.0, dir.x), step(0.0, dir.y), step(0.0, dir.z));

    vec3 tMax = vec3(
        abs(dir.x) > EPS ? (voxelBoundary.x - origin.x) / dir.x : 1e30,
        abs(dir.y) > EPS ? (voxelBoundary.y - origin.y) / dir.y : 1e30,
        abs(dir.z) > EPS ? (voxelBoundary.z - origin.z) / dir.z : 1e30
    );

    for (int i = 0; i < 1024; ++i) {
        int axis = 0;
        if (tMax.x < tMax.y) {
            axis = (tMax.x < tMax.z) ? 0 : 2;
        } else {
            axis = (tMax.y < tMax.z) ? 1 : 2;
        }

        float tHit = 0.0;
        if (axis == 0) {
            mapPos.x += stepSign.x;
            tHit = tMax.x;
            tMax.x += tDelta.x;
        } else if (axis == 1) {
            mapPos.y += stepSign.y;
            tHit = tMax.y;
            tMax.y += tDelta.y;
        } else {
            mapPos.z += stepSign.z;
            tHit = tMax.z;
            tMax.z += tDelta.z;
        }

        prevMapPos = mapPos - stepSign * ((axis==0) ? ivec3(1,0,0) : (axis==1 ? ivec3(0,1,0) : ivec3(0,0,1)));
        
        int currentIndex = getVoxelIndex(mapPos);
        int prevIndex = getVoxelIndex(prevMapPos);
        
        if (currentIndex >= 0) {
            float currentRefrac = voxels[currentIndex].voxel.refraction <= 0.0 ? 1.0 : voxels[currentIndex].voxel.refraction;
            float prevRefrac = (prevIndex >= 0 && voxels[prevIndex].voxel.refraction > 0.0) ? voxels[prevIndex].voxel.refraction : thisRay.IOF;

            bool refracChanged = abs(currentRefrac - prevRefrac) > 1e-4;

            if (refracChanged && tHit > 1e-5) {
                hitMapPos = mapPos;
                hitPoint = origin + dir * tHit;
                if (axis == 0) hitNormal = vec3(-float(stepSign.x), 0.0, 0.0);
                else if (axis == 1) hitNormal = vec3(0.0, -float(stepSign.y), 0.0);
                else hitNormal = vec3(0.0, 0.0, -float(stepSign.z));
                return true;
            }
        }
    }

    return false;
}

vec4 raymarching_fromDir(vec3 rayOrigin, vec3 rayDir) {
    ivec3 thisMapPos = ivec3(floor(rayOrigin));
    int thisVoxelIndex = getVoxelIndex(thisMapPos);
    float startIOF = 1.0;
    if (thisVoxelIndex >= 0 && voxels[thisVoxelIndex].voxel.refraction > 0.0) {
        startIOF = voxels[thisVoxelIndex].voxel.refraction;
    }

    Ray rayStack[MAX_RAYS];
    Ray resultRays[MAX_RAYS];
    for (int i = 0; i < MAX_RAYS; ++i){
        rayStack[i].defined = false;
        resultRays[i].defined = false;
        resultRays[i].weight = 0.0;
        resultRays[i].colorTint = vec4(0.0);
    }

    rayStack[0].origin = rayOrigin;
    rayStack[0].direction = normalize(rayDir);
    rayStack[0].IOF = startIOF;
    rayStack[0].defined = true;
    rayStack[0].weight = 1.0;
    rayStack[0].colorTint = vec4(1.0);
    rayStack[0].distanceInMedium = 0.0;
    rayStack[0].mediumColor = vec4(1.0);
    rayStack[0].mediumDensity = 0.0;
    int stackSize = 1;
    int resultSize = 0;

    while (stackSize > 0) {
        int rayIndex = stackSize - 1;
        Ray currentRay = rayStack[rayIndex];
        vec3 rayStartPoint = currentRay.origin;
        popRay(rayStack, stackSize);

        if (!currentRay.defined) continue;

        ivec3 mapPos;
        ivec3 prevMapPos;
        vec3 hitPoint;
        vec3 hitNormal;
        bool hit = hitMarching(currentRay, mapPos, prevMapPos, hitPoint, hitNormal);

        if (!hit) {
            Ray finalRay;
            finalRay.defined = true;
            finalRay.weight = currentRay.weight;
            
            // Apply Beer's Law absorption for the final segment
            vec4 transmittedColor = currentRay.colorTint;
            float distanceTraveled = currentRay.distanceInMedium;
            if (distanceTraveled > 1e-6 && currentRay.mediumDensity > 0.0) {
                vec3 absorption = exp(-currentRay.mediumDensity * distanceTraveled * (vec3(1.0) - currentRay.mediumColor.rgb));
                transmittedColor.rgb *= absorption;
            }
            
            // Blend with sky color
            finalRay.colorTint = transmittedColor * skyColor;
            addRay(resultRays, finalRay, resultSize);
            continue;
        }

        // Update distance traveled through current medium
        float segmentDistance = length(hitPoint - rayStartPoint);
        currentRay.distanceInMedium += segmentDistance;

        int index = getVoxelIndex(mapPos);
        VoxelObj thisVoxel;
        vec4 voxelColor;

        if (index >= 0) {
            thisVoxel = voxels[index];
            voxelColor = rgbaColor(thisVoxel.color);
        } else {
            thisVoxel.voxel.refraction = 1.0;
            thisVoxel.voxel.emmission = 0.0;
            thisVoxel.voxel.mettalicity = 0.0;
            voxelColor = vec4(0.0);
        }

        int prevIndex = getVoxelIndex(prevMapPos);
        VoxelObj lastVoxel;
        vec4 lastVoxelColor;

        if (prevIndex >= 0) {
            lastVoxel = voxels[prevIndex];
            lastVoxelColor = rgbaColor(lastVoxel.color);
        } else {
            lastVoxel.voxel.refraction = currentRay.IOF > 0.0 ? currentRay.IOF : 1.0;
            lastVoxel.voxel.emmission = 0.0;
            lastVoxel.voxel.mettalicity = 0.0;
            lastVoxelColor = vec4(0.0);
        }

        vec3 normal = hitNormal;
        if (length(normal) == 0.0) normal = vec3(0.0, 1.0, 0.0);

        vec3 lightDir = normalize(vec3(0.4, 1.0, 0.4));

        float ambientStrength = 0.2;
        vec3 ambient = vec3(ambientStrength);

        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = vec3(diff);

        // Shadow check
        Ray lightRay;
        lightRay.origin = hitPoint + normal * 1e-3;
        lightRay.direction = lightDir;
        lightRay.IOF = currentRay.IOF;
        lightRay.defined = true;

        ivec3 shadowHit, shadowPrev;
        vec3 shadowPoint, shadowNormal;
        if (hitMarching(lightRay, shadowHit, shadowPrev, shadowPoint, shadowNormal)) {
            diffuse = vec3(0.0);
        }

        vec3 finalLight = ambient + diffuse;
        
        // Determine surface color based on which side we're hitting
        vec4 surfaceColor;
        float surfaceAlpha;
        if (voxelColor.a > 0.0) {
            surfaceColor = voxelColor;
            surfaceAlpha = voxelColor.a;
        } else {
            surfaceColor = lastVoxelColor;
            surfaceAlpha = lastVoxelColor.a;
        }

        float refracIndex = thisVoxel.voxel.refraction > 0.0 ? thisVoxel.voxel.refraction : 1.0;
        float prevRefracIndex = lastVoxel.voxel.refraction > 0.0 ? lastVoxel.voxel.refraction : 1.0;

        vec3 incidentDir = normalize(currentRay.direction);
        vec3 n = normal;

        // Apply Beer's Law absorption for distance traveled so far
        vec4 transmittedColor = currentRay.colorTint;
        float distanceTraveled = currentRay.distanceInMedium;
        if (distanceTraveled > 1e-6 && currentRay.mediumDensity > 0.0) {
            vec3 absorption = exp(-currentRay.mediumDensity * distanceTraveled * (vec3(1.0) - currentRay.mediumColor.rgb));
            transmittedColor.rgb *= absorption;
        }

        // === TRANSPARENT/TRANSLUCENT SURFACES ===
        if (surfaceAlpha < 1) {
            float cosi = dot(incidentDir, n);
            float n1 = prevRefracIndex;
            float n2 = refracIndex;
            if (cosi > 0.0) {
                n = -n;
                float tmp = n1;
                n1 = n2;
                n2 = tmp;
            }
            float eta = n1 / n2;
            vec3 refractDir = refract(incidentDir, n, eta);
            vec3 reflectDir = reflect(incidentDir, n);

            float R0 = pow((n1 - n2) / (n1 + n2), 2.0);
            float cosTheta = max(0.0, dot(-incidentDir, n));
            float fresnel = R0 + (1.0 - R0) * pow(1.0 - cosTheta, 5.0);
            fresnel = clamp(fresnel, 0.0, 1.0);

            bool hasTIR = length(refractDir) < 0.001;
            float reflectIntensity = fresnel;
            float refractIntensity = hasTIR ? 0.0 : (1.0 - fresnel);

            // DONT WASTE ENERGY YOU SICK FUCK
            if(stackSize == MAX_RAYS || reflectIntensity <= 0.001 || refractIntensity <= 0.001){
                vec4 litSurfaceColor = vec4(surfaceColor.rgb * finalLight, 1.0);
                Ray finalRay;
                finalRay.defined = true;
                finalRay.weight = currentRay.weight;
                finalRay.colorTint = transmittedColor * litSurfaceColor;
                
                addRay(resultRays, finalRay, resultSize);
                continue;
            }

            // ---- Reflection ray ----
            if (reflectIntensity > 0.001 && stackSize < MAX_RAYS) {
                Ray reflectionRay;
                reflectionRay.direction = reflectDir;
                reflectionRay.origin = hitPoint + n * 1e-4;
                reflectionRay.defined = true;
                reflectionRay.IOF = n1;
                reflectionRay.weight = currentRay.weight * reflectIntensity;
                reflectionRay.colorTint = transmittedColor;
                
                // Going back into previous medium
                reflectionRay.distanceInMedium = 0.0;
                reflectionRay.mediumColor = lastVoxelColor;
                reflectionRay.mediumDensity = lastVoxelColor.a * 5.0;
                
                if (reflectionRay.weight > 1e-4)
                    addRay(rayStack, reflectionRay, stackSize);
            }

            // ---- Refraction ray ----
            if (refractIntensity > 0.001 && stackSize < MAX_RAYS && !hasTIR) {
                Ray refractionRay;
                refractionRay.direction = refractDir;
                refractionRay.origin = hitPoint - n * 1e-4;
                refractionRay.defined = true;
                refractionRay.IOF = n2;
                refractionRay.weight = currentRay.weight * refractIntensity;
                refractionRay.colorTint = transmittedColor;
                
                // Entering new medium
                refractionRay.distanceInMedium = 0.0;
                refractionRay.mediumColor = voxelColor;
                refractionRay.mediumDensity = voxelColor.a * 5.0;
                
                if (refractionRay.weight > 1e-4)
                    addRay(rayStack, refractionRay, stackSize);
            }
        }
        else {
            // === OPAQUE SURFACE ===
            float specularFactor = thisVoxel.voxel.mettalicity;
            float diffuseFactor = 1.0 - specularFactor;

            vec4 litSurfaceColor = vec4(surfaceColor.rgb * finalLight, 1.0);

            // Diffuse component
            if (diffuseFactor > 0.01) {
                Ray finalRay;
                finalRay.defined = true;
                finalRay.weight = currentRay.weight * diffuseFactor;
                finalRay.colorTint = transmittedColor * litSurfaceColor;
                
                addRay(resultRays, finalRay, resultSize);
            }

            // Specular reflection
            if (specularFactor > 0.01 && stackSize < MAX_RAYS) {
                vec3 reflectDir = reflect(incidentDir, normal);
                Ray reflectionRay;
                reflectionRay.direction = reflectDir;
                reflectionRay.origin = hitPoint + normal * 1e-4;
                reflectionRay.defined = true;
                reflectionRay.IOF = prevRefracIndex;
                reflectionRay.weight = currentRay.weight * specularFactor;
                reflectionRay.colorTint = transmittedColor;
                reflectionRay.distanceInMedium = 0.0;
                reflectionRay.mediumColor = vec4(1.0);
                reflectionRay.mediumDensity = 0.0;
                
                if (reflectionRay.weight > 1e-4)
                    addRay(rayStack, reflectionRay, stackSize);
            }
        }
    }
    
    return accumulateColors(resultRays);
}

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = imageSize(destTex);

    if (pixel_coords.x < 0 || pixel_coords.x >= dims.x || pixel_coords.y < 0 || pixel_coords.y >= dims.y) return;

    float u = (float(pixel_coords.x) / float(dims.x)) * 2.0 - 1.0;
    float v = (float(pixel_coords.y) / float(dims.y)) * 2.0 - 1.0;

    vec4 clip = vec4(u, v, -1.0, 1.0);
    vec4 view = invProjection * clip;
    if (abs(view.w) > 1e-6) view /= view.w;
    vec3 viewDir = normalize(view.xyz);
    vec3 worldDir = normalize((invView * vec4(viewDir, 0.0)).xyz);

    vec4 finalColor = raymarching_fromDir(cameraPos, worldDir);

    if (length(finalColor.rgb) == 0.0) finalColor = skyColor;

    imageStore(destTex, pixel_coords, finalColor);
}