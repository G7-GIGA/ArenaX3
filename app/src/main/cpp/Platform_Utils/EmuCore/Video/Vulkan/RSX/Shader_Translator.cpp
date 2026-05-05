// Shader_Translator.cpp
// ArenaX3 Shader Translation Module
// Copyright (c) 2024 ArenaX3 Project. All rights reserved.

#include "Shader_Translator.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace com {
namespace arenax3 {

// Static shader cache
std::unordered_map<std::string, std::string> Shader_Translator::m_shaderCache;
std::mutex Shader_Translator::m_cacheMutex;

// Constructor
Shader_Translator::Shader_Translator()
    : m_currentAPI(GraphicsAPI::OPENGL)
    , m_optimizationLevel(OptimizationLevel::BALANCED)
    , m_verboseLogging(false) {
}

// Destructor
Shader_Translator::~Shader_Translator() {
    clearCache();
}

// Set the target graphics API
void Shader_Translator::setGraphicsAPI(GraphicsAPI api) {
    std::lock_guard<std::mutex> lock(m_translationMutex);
    m_currentAPI = api;
    
    if (m_verboseLogging) {
        logMessage("Graphics API set to: " + std::to_string(static_cast<int>(api)));
    }
}

// Set optimization level
void Shader_Translator::setOptimizationLevel(OptimizationLevel level) {
    std::lock_guard<std::mutex> lock(m_translationMutex);
    m_optimizationLevel = level;
    
    if (m_verboseLogging) {
        logMessage("Optimization level set to: " + std::to_string(static_cast<int>(level)));
    }
}

// Enable/disable verbose logging
void Shader_Translator::setVerboseLogging(bool enable) {
    std::lock_guard<std::mutex> lock(m_translationMutex);
    m_verboseLogging = enable;
}

// Translate shader from source format to target API
bool Shader_Translator::translateShader(const ShaderSource& source, 
                                        ShaderSource& output, 
                                        std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_translationMutex);
    
    if (source.vertexSource.empty() && source.fragmentSource.empty()) {
        errorMsg = "Empty shader source provided";
        return false;
    }
    
    // Generate cache key
    std::string cacheKey = generateCacheKey(source);
    
    // Check cache first
    {
        std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
        auto it = m_shaderCache.find(cacheKey);
        if (it != m_shaderCache.end()) {
            output.cached = true;
            output.vertexSource = it->second + "_VERTEX";
            output.fragmentSource = it->second + "_FRAGMENT";
            output.metadata = source.metadata;
            output.metadata["translated_from_cache"] = "true";
            
            if (m_verboseLogging) {
                logMessage("Shader retrieved from cache");
            }
            return true;
        }
    }
    
    // Perform translation based on source format
    bool success = false;
    
    switch (source.format) {
        case ShaderFormat::GLSL:
            success = translateFromGLSL(source, output, errorMsg);
            break;
            
        case ShaderFormat::HLSL:
            success = translateFromHLSL(source, output, errorMsg);
            break;
            
        case ShaderFormat::SPIRV:
            success = translateFromSPIRV(source, output, errorMsg);
            break;
            
        case ShaderFormat::METAL:
            success = translateFromMetal(source, output, errorMsg);
            break;
            
        default:
            errorMsg = "Unsupported shader format";
            return false;
    }
    
    // Apply optimizations if successful
    if (success && m_optimizationLevel != OptimizationLevel::NONE) {
        applyOptimizations(output);
    }
    
    // Cache the result
    if (success && output.cacheable) {
        std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
        std::string combinedOutput = output.vertexSource + "|" + output.fragmentSource;
        m_shaderCache[cacheKey] = combinedOutput;
        
        if (m_verboseLogging) {
            logMessage("Shader cached with key: " + cacheKey);
        }
    }
    
    return success;
}

// Translate from GLSL to target API
bool Shader_Translator::translateFromGLSL(const ShaderSource& source, 
                                          ShaderSource& output, 
                                          std::string& errorMsg) {
    if (m_verboseLogging) {
        logMessage("Translating from GLSL");
    }
    
    output.format = source.format;
    output.metadata = source.metadata;
    output.metadata["source_api"] = "OpenGL";
    output.cacheable = true;
    
    switch (m_currentAPI) {
        case GraphicsAPI::OPENGL:
            // No translation needed for GLSL to OpenGL
            output.vertexSource = source.vertexSource;
            output.fragmentSource = source.fragmentSource;
            output.metadata["target_api"] = "OpenGL";
            return true;
            
        case GraphicsAPI::VULKAN:
            // Convert GLSL to SPIR-V compatible format
            output.vertexSource = convertGLSLToVulkan(source.vertexSource, true);
            output.fragmentSource = convertGLSLToVulkan(source.fragmentSource, false);
            output.metadata["target_api"] = "Vulkan";
            return true;
            
        case GraphicsAPI::DIRECTX12:
            // Convert GLSL to HLSL
            output.vertexSource = convertGLSLToHLSL(source.vertexSource, true);
            output.fragmentSource = convertGLSLToHLSL(source.fragmentSource, false);
            output.metadata["target_api"] = "DirectX12";
            return true;
            
        case GraphicsAPI::METAL:
            // Convert GLSL to Metal Shading Language
            output.vertexSource = convertGLSLToMetal(source.vertexSource, true);
            output.fragmentSource = convertGLSLToMetal(source.fragmentSource, false);
            output.metadata["target_api"] = "Metal";
            return true;
            
        default:
            errorMsg = "Unsupported target graphics API for GLSL translation";
            return false;
    }
}

// Translate from HLSL to target API
bool Shader_Translator::translateFromHLSL(const ShaderSource& source, 
                                          ShaderSource& output, 
                                          std::string& errorMsg) {
    if (m_verboseLogging) {
        logMessage("Translating from HLSL");
    }
    
    output.format = source.format;
    output.metadata = source.metadata;
    output.metadata["source_api"] = "DirectX";
    output.cacheable = true;
    
    switch (m_currentAPI) {
        case GraphicsAPI::OPENGL:
            output.vertexSource = convertHLSLToGLSL(source.vertexSource, true);
            output.fragmentSource = convertHLSLToGLSL(source.fragmentSource, false);
            output.metadata["target_api"] = "OpenGL";
            return true;
            
        case GraphicsAPI::VULKAN:
            output.vertexSource = convertHLSLToSPIRV(source.vertexSource, true);
            output.fragmentSource = convertHLSLToSPIRV(source.fragmentSource, false);
            output.metadata["target_api"] = "Vulkan";
            return true;
            
        case GraphicsAPI::DIRECTX12:
            output.vertexSource = source.vertexSource;
            output.fragmentSource = source.fragmentSource;
            output.metadata["target_api"] = "DirectX12";
            return true;
            
        case GraphicsAPI::METAL:
            output.vertexSource = convertHLSLToMetal(source.vertexSource, true);
            output.fragmentSource = convertHLSLToMetal(source.fragmentSource, false);
            output.metadata["target_api"] = "Metal";
            return true;
            
        default:
            errorMsg = "Unsupported target graphics API for HLSL translation";
            return false;
    }
}

// Translate from SPIR-V to target API
bool Shader_Translator::translateFromSPIRV(const ShaderSource& source, 
                                           ShaderSource& output, 
                                           std::string& errorMsg) {
    if (m_verboseLogging) {
        logMessage("Translating from SPIR-V");
    }
    
    output.format = source.format;
    output.metadata = source.metadata;
    output.metadata["source_api"] = "SPIR-V";
    output.cacheable = true;
    
    switch (m_currentAPI) {
        case GraphicsAPI::VULKAN:
            output.vertexSource = source.vertexSource;
            output.fragmentSource = source.fragmentSource;
            output.metadata["target_api"] = "Vulkan";
            return true;
            
        case GraphicsAPI::OPENGL:
            output.vertexSource = convertSPIRVToGLSL(source.vertexSource, true);
            output.fragmentSource = convertSPIRVToGLSL(source.fragmentSource, false);
            output.metadata["target_api"] = "OpenGL";
            return true;
            
        default:
            errorMsg = "SPIR-V translation only supported for OpenGL and Vulkan targets";
            return false;
    }
}

// Translate from Metal to target API
bool Shader_Translator::translateFromMetal(const ShaderSource& source, 
                                           ShaderSource& output, 
                                           std::string& errorMsg) {
    if (m_verboseLogging) {
        logMessage("Translating from Metal");
    }
    
    output.format = source.format;
    output.metadata = source.metadata;
    output.metadata["source_api"] = "Metal";
    output.cacheable = true;
    
    switch (m_currentAPI) {
        case GraphicsAPI::METAL:
            output.vertexSource = source.vertexSource;
            output.fragmentSource = source.fragmentSource;
            output.metadata["target_api"] = "Metal";
            return true;
            
        case GraphicsAPI::OPENGL:
            output.vertexSource = convertMetalToGLSL(source.vertexSource, true);
            output.fragmentSource = convertMetalToGLSL(source.fragmentSource, false);
            output.metadata["target_api"] = "OpenGL";
            return true;
            
        default:
            errorMsg = "Metal translation only supported for OpenGL and Metal targets";
            return false;
    }
}

// Apply optimizations to translated shader
void Shader_Translator::applyOptimizations(ShaderSource& shader) {
    if (m_optimizationLevel == OptimizationLevel::NONE) {
        return;
    }
    
    if (m_verboseLogging) {
        logMessage("Applying optimizations (level: " + 
                   std::to_string(static_cast<int>(m_optimizationLevel)) + ")");
    }
    
    // Remove comments
    if (m_optimizationLevel >= OptimizationLevel::BASIC) {
        shader.vertexSource = removeComments(shader.vertexSource);
        shader.fragmentSource = removeComments(shader.fragmentSource);
    }
    
    // Remove unnecessary whitespace
    if (m_optimizationLevel >= OptimizationLevel::BASIC) {
        shader.vertexSource = minifyWhitespace(shader.vertexSource);
        shader.fragmentSource = minifyWhitespace(shader.fragmentSource);
    }
    
    // Inline constant expressions
    if (m_optimizationLevel >= OptimizationLevel::AGGRESSIVE) {
        shader.vertexSource = inlineConstants(shader.vertexSource);
        shader.fragmentSource = inlineConstants(shader.fragmentSource);
    }
    
    // Remove dead code
    if (m_optimizationLevel >= OptimizationLevel::AGGRESSIVE) {
        shader.vertexSource = removeDeadCode(shader.vertexSource);
        shader.fragmentSource = removeDeadCode(shader.fragmentSource);
    }
    
    shader.metadata["optimization_applied"] = "true";
}

// Clear translation cache
void Shader_Translator::clearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_shaderCache.clear();
    
    if (m_verboseLogging) {
        logMessage("Shader cache cleared");
    }
}

// Get cache size
size_t Shader_Translator::getCacheSize() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_shaderCache.size();
}

// Generate cache key from shader source
std::string Shader_Translator::generateCacheKey(const ShaderSource& source) const {
    std::string combined = source.vertexSource + "|" + source.fragmentSource + "|" +
                          std::to_string(static_cast<int>(source.format)) + "|" +
                          std::to_string(static_cast<int>(m_currentAPI)) + "|" +
                          std::to_string(static_cast<int>(m_optimizationLevel));
    
    // Simple hash for cache key (in production, use a proper hash)
    size_t hash = std::hash<std::string>{}(combined);
    return std::to_string(hash);
}

// Log message
void Shader_Translator::logMessage(const std::string& message) {
    if (m_verboseLogging) {
        // In production, this would write to a proper logging system
        // For now, we'll just output to stderr
        fprintf(stderr, "[Shader_Translator] %s\n", message.c_str());
    }
}

// ============================================================================
// Conversion helper functions (simplified implementations)
// ============================================================================

std::string Shader_Translator::convertGLSLToVulkan(const std::string& glsl, bool isVertex) {
    std::string result = glsl;
    
    // Add Vulkan-specific extensions
    result = "#version 450\n" + result;
    
    // Convert layout qualifiers for Vulkan
    std::regex locationRegex(R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+)");
    result = std::regex_replace(result, locationRegex, "layout(location = $1) in ");
    
    // Convert uniform blocks to Vulkan-style descriptor sets
    std::regex uniformBlockRegex(R"(uniform\s+(\w+)\s*\{)");
    result = std::regex_replace(result, uniformBlockRegex, "layout(set = 0, binding = 0) uniform $1 {");
    
    return result;
}

std::string Shader_Translator::convertGLSLToHLSL(const std::string& glsl, bool isVertex) {
    std::string result = glsl;
    
    // Replace GLSL-specific keywords with HLSL equivalents
    std::regex vec3Regex(R"(\bvec3\b)");
    result = std::regex_replace(result, vec3Regex, "float3");
    
    std::regex vec4Regex(R"(\bvec4\b)");
    result = std::regex_replace(result, vec4Regex, "float4");
    
    std::regex mat4Regex(R"(\bmat4\b)");
    result = std::regex_replace(result, mat4Regex, "float4x4");
    
    // Convert attribute/uniform semantics
    if (isVertex) {
        std::regex attributeRegex(R"(in\s+(\w+)\s+(\w+);)");
        result = std::regex_replace(result, attributeRegex, "float4 $2 : $1;");
    }
    
    return result;
}

std::string Shader_Translator::convertGLSLToMetal(const std::string& glsl, bool isVertex) {
    std::string result = glsl;
    
    // Add Metal-specific headers
    result = "#include <metal_stdlib>\nusing namespace metal;\n\n" + result;
    
    // Convert GLSL types to Metal types
    std::regex vec3Regex(R"(\bvec3\b)");
    result = std::regex_replace(result, vec3Regex, "float3");
    
    std::regex vec4Regex(R"(\bvec4\b)");
    result = std::regex_replace(result, vec4Regex, "float4");
    
    // Convert main function signature
    if (isVertex) {
        result = std::regex_replace(result, std::regex(R"(void\s+main\s*\()"), 
                                   "vertex void main_vertex(");
    } else {
        result = std::regex_replace(result, std::regex(R"(void\s+main\s*\()"), 
                                   "fragment void main_fragment(");
    }
    
    return result;
}

std::string Shader_Translator::convertHLSLToGLSL(const std::string& hlsl, bool isVertex) {
    std::string result = hlsl;
    
    // Replace HLSL-specific keywords
    std::regex float3Regex(R"(\bfloat3\b)");
    result = std::regex_replace(result, float3Regex, "vec3");
    
    std::regex float4Regex(R"(\bfloat4\b)");
    result = std::regex_replace(result, float4Regex, "vec4");
    
    std::regex float4x4Regex(R"(\bfloat4x4\b)");
    result = std::regex_replace(result, float4x4Regex, "mat4");
    
    // Convert semantics to layout qualifiers
    std::regex semanticRegex(R"(:\\s+\\w+;)");
    result = std::regex_replace(result, semanticRegex, ";");
    
    return result;
}

std::string Shader_Translator::convertHLSLToSPIRV(const std::string& hlsl, bool isVertex) {
    // This would typically use DXC or similar compiler
    // For demonstration, we return a placeholder
    return "// SPIR-V binary would be here\n// Converted from HLSL";
}

std::string Shader_Translator::convertHLSLToMetal(const std::string& hlsl, bool isVertex) {
    std::string result = hlsl;
    
    // Basic type conversions
    std::regex float3Regex(R"(\bfloat3\b)");
    result = std::regex_replace(result, float3Regex, "float3");
    
    std::regex float4Regex(R"(\bfloat4\b)");
    result = std::regex_replace(result, float4Regex, "float4");
    
    return result;
}

std::string Shader_Translator::convertSPIRVToGLSL(const std::string& spirv, bool isVertex) {
    // This would use SPIRV-Cross or similar
    // For demonstration, we return a placeholder
    return "// GLSL converted from SPIR-V\n// Original SPIR-V binary";
}

std::string Shader_Translator::convertMetalToGLSL(const std::string& metal, bool isVertex) {
    std::string result = metal;
    
    // Metal to GLSL type conversions
    std::regex metalFloat3Regex(R"(\bfloat3\b)");
    result = std::regex_replace(result, metalFloat3Regex, "vec3");
    
    std::regex metalFloat4Regex(R"(\bfloat4\b)");
    result = std::regex_replace(result, metalFloat4Regex, "vec4");
    
    return result;
}

std::string Shader_Translator::removeComments(const std::string& shader) {
    std::string result;
    bool inLineComment = false;
    bool inBlockComment = false;
    
    for (size_t i = 0; i < shader.length(); ++i) {
        if (!inBlockComment && !inLineComment && shader[i] == '/' && i + 1 < shader.length()) {
            if (shader[i + 1] == '/') {
                inLineComment = true;
                i++;
                continue;
            } else if (shader[i + 1] == '*') {
                inBlockComment = true;
                i++;
                continue;
            }
        }
        
        if (inLineComment && shader[i] == '\n') {
            inLineComment = false;
            result += '\n';
            continue;
        }
        
        if (inBlockComment && shader[i] == '*' && i + 1 < shader.length() && shader[i + 1] == '/') {
            inBlockComment = false;
            i++;
            continue;
        }
        
        if (!inLineComment && !inBlockComment) {
            result += shader[i];
        }
    }
    
    return result;
}

std::string Shader_Translator::minifyWhitespace(const std::string& shader) {
    std::string result;
    bool lastWasWhitespace = false;
    
    for (char c : shader) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!lastWasWhitespace) {
                result += ' ';
                lastWasWhitespace = true;
            }
        } else {
            result += c;
            lastWasWhitespace = false;
        }
    }
    
    return result;
}

std::string Shader_Translator::inlineConstants(const std::string& shader) {
    // Simplified constant inlining
    std::string result = shader;
    std::regex constRegex(R"(const\s+\w+\s+(\w+)\s*=\s*([^;]+);)");
    std::smatch match;
    
    while (std::regex_search(result, match, constRegex)) {
        std::string constName = match[1].str();
        std::string constValue = match[2].str();
        
        std::regex useRegex("\\b" + constName + "\\b");
        result = std::regex_replace(result, useRegex, constValue);
        result = std::regex_replace(result, constRegex, "");
    }
    
    return result;
}

std::string Shader_Translator::removeDeadCode(const std::string& shader) {
    // Simplified dead code elimination
    std::string result = shader;
    
    // Remove unreachable code after return statements (simplified)
    std::regex returnRegex(R"(return[^;]*;[\s]*([^}]*))");
    result = std::regex_replace(result, returnRegex, "return$1;");
    
    return result;
}

// Helper function to get graphics API name
std::string Shader_Translator::getGraphicsAPIName(GraphicsAPI api) const {
    switch (api) {
        case GraphicsAPI::OPENGL: return "OpenGL";
        case GraphicsAPI::VULKAN: return "Vulkan";
        case GraphicsAPI::DIRECTX12: return "DirectX 12";
        case GraphicsAPI::METAL: return "Metal";
        default: return "Unknown";
    }
}

} // namespace arenax3
} // namespace com