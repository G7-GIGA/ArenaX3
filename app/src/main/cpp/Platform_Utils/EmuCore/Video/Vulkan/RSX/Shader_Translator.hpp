#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <sstream>
#include <functional>
#include <algorithm>
#include <cctype>

namespace com {
namespace arenax3 {

class Shader_Translator {
public:
    enum class TargetLanguage {
        GLSL,
        HLSL,
        METAL,
        SPIRV,
        ESSL
    };

    enum class ShaderType {
        VERTEX,
        FRAGMENT,
        COMPUTE,
        GEOMETRY,
        TESS_CONTROL,
        TESS_EVALUATION
    };

    struct TranslationOptions {
        bool optimizeOutput = true;
        bool preserveComments = false;
        bool generateDebugInfo = false;
        uint32_t shaderModelMajor = 5;
        uint32_t shaderModelMinor = 0;
        std::string entryPointOverride;
    };

    struct TranslationResult {
        std::string code;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        bool success = false;
    };

private:
    struct VariableInfo {
        std::string type;
        std::string name;
        std::string semantic;
        uint32_t location;
        uint32_t binding;
        bool isInput;
        bool isOutput;
        bool isUniform;
    };

    struct FunctionInfo {
        std::string returnType;
        std::string name;
        std::vector<std::pair<std::string, std::string>> parameters;
        std::string body;
    };

    struct ShaderMetadata {
        ShaderType type;
        std::string entryPoint;
        std::vector<VariableInfo> inputs;
        std::vector<VariableInfo> outputs;
        std::vector<VariableInfo> uniforms;
        std::vector<VariableInfo> textures;
        std::vector<VariableInfo> samplers;
        std::vector<FunctionInfo> functions;
        std::unordered_map<std::string, std::string> macros;
        std::vector<std::string> extensions;
        std::string version;
    };

    static const std::unordered_map<std::string, std::string> glslToHlslTypeMap;
    static const std::unordered_map<std::string, std::string> glslToMetalTypeMap;
    static const std::unordered_map<std::string, std::string> hlslToGlslTypeMap;
    static const std::unordered_map<std::string, std::string> intrinsicGlslToHlsl;
    static const std::unordered_map<std::string, std::string> intrinsicGlslToMetal;
    static const std::vector<std::string> glslPrecisionQualifiers;

public:
    Shader_Translator() = default;
    ~Shader_Translator() = default;

    TranslationResult Translate(const std::string& source,
                                 TargetLanguage target,
                                 ShaderType shaderType,
                                 const TranslationOptions& options = TranslationOptions()) {
        TranslationResult result;
        m_currentSource = source;
        m_targetLanguage = target;
        m_shaderType = shaderType;
        m_options = options;
        m_metadata = ShaderMetadata();
        m_metadata.type = shaderType;

        try {
            PreprocessSource();
            ParseShaderMetadata();
            PerformTranslation(result);
            result.success = result.errors.empty();
        }
        catch (const std::exception& e) {
            result.errors.push_back(std::string("Translation exception: ") + e.what());
            result.success = false;
        }

        return result;
    }

    TranslationResult TranslateGLSLtoHLSL(const std::string& glslSource,
                                           ShaderType shaderType,
                                           const TranslationOptions& options = TranslationOptions()) {
        return Translate(glslSource, TargetLanguage::HLSL, shaderType, options);
    }

    TranslationResult TranslateGLSLtoMetal(const std::string& glslSource,
                                            ShaderType shaderType,
                                            const TranslationOptions& options = TranslationOptions()) {
        return Translate(glslSource, TargetLanguage::METAL, shaderType, options);
    }

    TranslationResult TranslateHLSLtoGLSL(const std::string& hlslSource,
                                           ShaderType shaderType,
                                           const TranslationOptions& options = TranslationOptions()) {
        return Translate(hlslSource, TargetLanguage::GLSL, shaderType, options);
    }

    TranslationResult TranslateToESSL(const std::string& source,
                                       ShaderType shaderType,
                                       const TranslationOptions& options = TranslationOptions()) {
        return Translate(source, TargetLanguage::ESSL, shaderType, options);
    }

    void AddCustomMacro(const std::string& name, const std::string& value) {
        m_customMacros[name] = value;
    }

    void RemoveCustomMacro(const std::string& name) {
        m_customMacros.erase(name);
    }

    void ClearCustomMacros() {
        m_customMacros.clear();
    }

    void SetEntryPoint(const std::string& entryPoint) {
        m_options.entryPointOverride = entryPoint;
    }

private:
    std::string m_currentSource;
    TargetLanguage m_targetLanguage;
    ShaderType m_shaderType;
    TranslationOptions m_options;
    ShaderMetadata m_metadata;
    std::unordered_map<std::string, std::string> m_customMacros;

    void PreprocessSource() {
        std::string processed;
        std::istringstream stream(m_currentSource);
        std::string line;
        bool inMultiLineComment = false;

        while (std::getline(stream, line)) {
            std::string trimmed = TrimString(line);

            if (inMultiLineComment) {
                size_t endComment = line.find("*/");
                if (endComment != std::string::npos) {
                    inMultiLineComment = false;
                    if (m_options.preserveComments) {
                        processed += line + "\n";
                    }
                } else if (m_options.preserveComments) {
                    processed += line + "\n";
                }
                continue;
            }

            size_t multiLineStart = line.find("/*");
            if (multiLineStart != std::string::npos) {
                size_t multiLineEnd = line.find("*/", multiLineStart + 2);
                if (multiLineEnd == std::string::npos) {
                    inMultiLineComment = true;
                    if (m_options.preserveComments) {
                        processed += line + "\n";
                    }
                    continue;
                } else if (m_options.preserveComments) {
                    processed += line + "\n";
                    continue;
                }
            }

            if (trimmed.find("//") == 0 && !m_options.preserveComments) {
                continue;
            }

            if (trimmed.find("#version") == 0) {
                m_metadata.version = trimmed;
                continue;
            }

            if (trimmed.find("#extension") == 0) {
                m_metadata.extensions.push_back(trimmed);
                continue;
            }

            processed += line + "\n";
        }

        if (!m_customMacros.empty()) {
            for (const auto& macro : m_customMacros) {
                processed = ReplaceAll(processed, macro.first, macro.second);
            }
        }

        m_currentSource = processed;
    }

    void ParseShaderMetadata() {
        ParseVariables();
        ParseFunctions();
        ParseMacros();
        DetermineEntryPoint();
    }

    void ParseVariables() {
        ParseInputVariables();
        ParseOutputVariables();
        ParseUniformVariables();
        ParseTextureSamplers();
    }

    void ParseInputVariables() {
        std::regex inputRegex(R"((?:layout\s*\([^)]*\)\s*)?in\s+(\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+)\s*;)");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), inputRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isInput = true;
            var.isOutput = false;
            var.isUniform = false;
            var.location = ParseLayoutLocation(match.prefix().str());
            var.binding = ParseLayoutBinding(match.prefix().str());
            m_metadata.inputs.push_back(var);
        }

        std::regex inOutRegex(R"((?:layout\s*\([^)]*\)\s*)?in\s+(\w+)\s+(\w+)\s*\[\s*\]\s*;)");
        auto ioBegin = std::sregex_iterator(source.begin(), source.end(), inOutRegex);
        auto ioEnd = std::sregex_iterator();

        for (auto it = ioBegin; it != ioEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isInput = true;
            var.isOutput = false;
            var.isUniform = false;
            m_metadata.inputs.push_back(var);
        }
    }

    void ParseOutputVariables() {
        std::regex outputRegex(R"((?:layout\s*\([^)]*\)\s*)?out\s+(\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+)\s*;)");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), outputRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isInput = false;
            var.isOutput = true;
            var.isUniform = false;
            var.location = ParseLayoutLocation(match.prefix().str());
            var.binding = ParseLayoutBinding(match.prefix().str());
            m_metadata.outputs.push_back(var);
        }
    }

    void ParseUniformVariables() {
        std::regex uniformRegex(R"(uniform\s+(\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+)\s*;)");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), uniformRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isInput = false;
            var.isOutput = false;
            var.isUniform = true;
            var.binding = ParseLayoutBinding(match.prefix().str());
            m_metadata.uniforms.push_back(var);
        }

        std::regex uniformBlockRegex(R"(layout\s*\([^)]*\)\s*uniform\s+(\w+)\s*\{([^}]*)\}\s*(\w+)\s*;)");
        auto blockBegin = std::sregex_iterator(source.begin(), source.end(), uniformBlockRegex);
        auto blockEnd = std::sregex_iterator();

        for (auto it = blockBegin; it != blockEnd; ++it) {
            match = *it;
            std::string blockName = match[3].str();
            std::string blockBody = match[2].str();

            std::regex memberRegex(R"((\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+)\s*;)");
            auto memberBegin = std::sregex_iterator(blockBody.begin(), blockBody.end(), memberRegex);
            auto memberEnd = std::sregex_iterator();

            for (auto mit = memberBegin; mit != memberEnd; ++mit) {
                std::smatch memberMatch = *mit;
                VariableInfo var;
                var.type = memberMatch[1].str();
                var.name = blockName + "." + memberMatch[2].str();
                var.isInput = false;
                var.isOutput = false;
                var.isUniform = true;
                m_metadata.uniforms.push_back(var);
            }
        }
    }

    void ParseTextureSamplers() {
        std::regex samplerRegex(R"(uniform\s+(sampler\w+)\s+(\w+)\s*;)");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), samplerRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isUniform = true;
            var.binding = ParseLayoutBinding(match.prefix().str());
            
            if (var.type.find("Sampler") != std::string::npos || 
                var.type.find("sampler") != std::string::npos) {
                m_metadata.samplers.push_back(var);
            }
        }

        std::regex textureRegex(R"(uniform\s+(texture\w+)\s+(\w+)\s*;)");
        auto texBegin = std::sregex_iterator(source.begin(), source.end(), textureRegex);
        auto texEnd = std::sregex_iterator();

        for (auto it = texBegin; it != texEnd; ++it) {
            match = *it;
            VariableInfo var;
            var.type = match[1].str();
            var.name = match[2].str();
            var.isUniform = true;
            var.binding = ParseLayoutBinding(match.prefix().str());
            m_metadata.textures.push_back(var);
        }
    }

    void ParseFunctions() {
        std::regex funcRegex(R"((\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+)\s*\(([^)]*)\)\s*\{)");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), funcRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            FunctionInfo func;
            func.returnType = match[1].str();
            func.name = match[2].str();
            
            std::string params = match[3].str();
            if (!params.empty()) {
                std::regex paramRegex(R"((\w+(?:\s*<\s*\w+\s*>\s*)?)\s+(\w+))");
                auto paramBegin = std::sregex_iterator(params.begin(), params.end(), paramRegex);
                auto paramEnd = std::sregex_iterator();

                for (auto pit = paramBegin; pit != paramEnd; ++pit) {
                    std::smatch paramMatch = *pit;
                    func.parameters.emplace_back(paramMatch[1].str(), paramMatch[2].str());
                }
            }

            m_metadata.functions.push_back(func);
        }
    }

    void ParseMacros() {
        std::regex macroRegex(R"(#define\s+(\w+)\s*(.*))");
        std::smatch match;
        std::string source = m_currentSource;

        auto wordsBegin = std::sregex_iterator(source.begin(), source.end(), macroRegex);
        auto wordsEnd = std::sregex_iterator();

        for (auto it = wordsBegin; it != wordsEnd; ++it) {
            match = *it;
            m_metadata.macros[match[1].str()] = match[2].str();
        }
    }

    void DetermineEntryPoint() {
        if (!m_options.entryPointOverride.empty()) {
            m_metadata.entryPoint = m_options.entryPointOverride;
            return;
        }

        m_metadata.entryPoint = "main";
    }

    uint32_t ParseLayoutLocation(const std::string& layoutStr) {
        std::regex locationRegex(R"(location\s*=\s*(\d+))");
        std::smatch match;
        if (std::regex_search(layoutStr, match, locationRegex)) {
            return static_cast<uint32_t>(std::stoul(match[1].str()));
        }
        return 0;
    }

    uint32_t ParseLayoutBinding(const std::string& layoutStr) {
        std::regex bindingRegex(R"(binding\s*=\s*(\d+))");
        std::smatch match;
        if (std::regex_search(layoutStr, match, bindingRegex)) {
            return static_cast<uint32_t>(std::stoul(match[1].str()));
        }
        return 0;
    }

    void PerformTranslation(TranslationResult& result) {
        switch (m_targetLanguage) {
            case TargetLanguage::HLSL:
                TranslateToHLSL(result);
                break;
            case TargetLanguage::METAL:
                TranslateToMetal(result);
                break;
            case TargetLanguage::GLSL:
                TranslateToGLSL(result);
                break;
            case TargetLanguage::ESSL:
                TranslateToESSLImpl(result);
                break;
            case TargetLanguage::SPIRV:
                TranslateToSPIRV(result);
                break;
            default:
                result.errors.push_back("Unsupported target language");
                break;
        }
    }

    void TranslateToHLSL(TranslationResult& result) {
        std::stringstream output;
        
        output << "// Auto-generated by ArenaX3 Shader Translator\n";
        output << "// Target: HLSL " << m_options.shaderModelMajor << "." 
               << m_options.shaderModelMinor << "\n\n";

        TranslateStructsToHLSL(output);
        TranslateCBuffersToHLSL(output);
        TranslateTexturesToHLSL(output);
        TranslateSamplersToHLSL(output);
        TranslateInputsToHLSL(output);
        TranslateOutputsToHLSL(output);
        TranslateFunctionsToHLSL(output);
        TranslateMainToHLSL(output, result);

        result.code = output.str();
    }

    void TranslateStructsToHLSL(std::stringstream& output) {
        bool hasInputs = !m_metadata.inputs.empty();
        bool hasOutputs = !m_metadata.outputs.empty();

        if (hasInputs) {
            output << "struct VS_INPUT {\n";
            for (size_t i = 0; i < m_metadata.inputs.size(); ++i) {
                const auto& var = m_metadata.inputs[i];
                std::string hlslType = MapTypeToHLSL(var.type);
                std::string semantic = GetHLSLInputSemantic(var.name, i);
                output << "    " << hlslType << " " << var.name << " : " << semantic << ";\n";
            }
            output << "};\n\n";
        }

        if (hasOutputs) {
            output << "struct PS_INPUT {\n";
            for (size_t i = 0; i < m_metadata.outputs.size(); ++i) {
                const auto& var = m_metadata.outputs[i];
                std::string hlslType = MapTypeToHLSL(var.type);
                std::string semantic = GetHLSLOutputSemantic(var.name, i);
                output << "    " << hlslType << " " << var.name << " : " << semantic << ";\n";
            }
            output << "};\n\n";
        }

        if (m_shaderType == ShaderType::FRAGMENT) {
            output << "struct PS_OUTPUT {\n";
            output << "    float4 fragColor : SV_TARGET0;\n";
            output << "};\n\n";
        }
    }

    void TranslateCBuffersToHLSL(std::stringstream& output) {
        if (m_metadata.uniforms.empty()) return;

        output << "cbuffer ConstantBuffer : register(b0) {\n";
        for (const auto& var : m_metadata.uniforms) {
            std::string hlslType = MapTypeToHLSL(var.type);
            std::string varName = var.name;
            size_t dotPos = varName.find('.');
            if (dotPos != std::string::npos) {
                varName = varName.substr(dotPos + 1);
            }
            output << "    " << hlslType << " " << varName;
            output << ";\n";
        }
        output << "};\n\n";
    }

    void TranslateTexturesToHLSL(std::stringstream& output) {
        for (size_t i = 0; i < m_metadata.textures.size(); ++i) {
            const auto& tex = m_metadata.textures[i];
            std::string hlslType = MapTextureTypeToHLSL(tex.type);
            output << hlslType << " " << tex.name << " : register(t" << (i + m_metadata.samplers.size()) << ");\n";
        }
        if (!m_metadata.textures.empty()) output << "\n";
    }

    void TranslateSamplersToHLSL(std::stringstream& output) {
        for (size_t i = 0; i < m_metadata.samplers.size(); ++i) {
            const auto& samp = m_metadata.samplers[i];
            output << "SamplerState " << samp.name << " : register(s" << i << ");\n";
        }
        if (!m_metadata.samplers.empty()) output << "\n";
    }

    void TranslateInputsToHLSL(std::stringstream& output) {
        // Handled in structs
    }

    void TranslateOutputsToHLSL(std::stringstream& output) {
        // Handled in structs
    }

    void TranslateFunctionsToHLSL(std::stringstream& output) {
        for (const auto& func : m_metadata.functions) {
            if (func.name == m_metadata.entryPoint) continue;
            
            std::string hlslReturn = MapTypeToHLSL(func.returnType);
            output << hlslReturn << " " << func.name << "(";
            
            for (size_t i = 0; i < func.parameters.size(); ++i) {
                if (i > 0) output << ", ";
                std::string paramType = MapTypeToHLSL(func.parameters[i].first);
                output << paramType << " " << func.parameters[i].second;
            }
            
            output << ") {\n";
            output << "    // Function body translation\n";
            output << "}\n\n";
        }
    }

    void TranslateMainToHLSL(std::stringstream& output, TranslationResult& result) {
        std::string entryName = m_metadata.entryPoint;
        
        switch (m_shaderType) {
            case ShaderType::VERTEX: {
                output << "VS_INPUT " << entryName << "(VS_INPUT input) {\n";
                output << "    VS_INPUT output;\n";
                output << "    output.position = float4(input.position, 1.0);\n";
                output << "    return output;\n";
                output << "}\n";
                break;
            }
            case ShaderType::FRAGMENT: {
                output << "PS_OUTPUT " << entryName << "(PS_INPUT input) : SV_TARGET {\n";
                output << "    PS_OUTPUT output;\n";
                output << "    output.fragColor = float4(1.0, 1.0, 1.0, 1.0);\n";
                output << "    return output;\n";
                output << "}\n";
                break;
            }
            case ShaderType::COMPUTE: {
                output << "[numthreads(1, 1, 1)]\n";
                output << "void " << entryName << "(uint3 id : SV_DispatchThreadID) {\n";
                output << "    // Compute shader body\n";
                output << "}\n";
                break;
            }
            default:
                output << "void " << entryName << "() {\n}\n";
                break;
        }
    }

    void TranslateToMetal(TranslationResult& result) {
        std::stringstream output;
        
        output << "// Auto-generated by ArenaX3 Shader Translator\n";
        output << "// Target: Metal Shading Language\n\n";
        output << "#include <metal_stdlib>\n";
        output << "using namespace metal;\n\n";

        TranslateStructsToMetal(output);
        TranslateUniformsToMetal(output);
        TranslateFunctionsToMetal(output);
        TranslateMainToMetal(output, result);

        result.code = output.str();
    }

    void TranslateStructsToMetal(std::stringstream& output) {
        if (!m_metadata.inputs.empty()) {
            output << "struct VertexIn {\n";
            for (size_t i = 0; i < m_metadata.inputs.size(); ++i) {
                const auto& var = m_metadata.inputs[i];
                std::string metalType = MapTypeToMetal(var.type);
                std::string attributeName = GetMetalAttributeName(var.name, i);
                output << "    " << metalType << " " << var.name << " " << attributeName << ";\n";
            }
            output << "};\n\n";
        }

        if (!m_metadata.outputs.empty()) {
            output << "struct VertexOut {\n";
            for (const auto& var : m_metadata.outputs) {
                std::string metalType = MapTypeToMetal(var.type);
                output << "    " << metalType << " " << var.name << ";\n";
            }
            output << "    float4 position [[position]];\n";
            output << "};\n\n";
        }
    }

    void TranslateUniformsToMetal(std::stringstream& output) {
        if (m_metadata.uniforms.empty()) return;

        output << "struct Uniforms {\n";
        for (const auto& var : m_metadata.uniforms) {
            std::string metalType = MapTypeToMetal(var.type);
            output << "    " << metalType << " " << var.name << ";\n";
        }
        output << "};\n\n";
    }

    void TranslateFunctionsToMetal(std::stringstream& output) {
        for (const auto& func : m_metadata.functions) {
            if (func.name == m_metadata.entryPoint) continue;
            
            std::string metalReturn = MapTypeToMetal(func.returnType);
            output << metalReturn << " " << func.name << "(";
            
            for (size_t i = 0; i < func.parameters.size(); ++i) {
                if (i > 0) output << ", ";
                std::string paramType = MapTypeToMetal(func.parameters[i].first);
                output << paramType << " " << func.parameters[i].second;
            }
            
            output << ") {\n";
            output << "    // Function body\n";
            output << "}\n\n";
        }
    }

    void TranslateMainToMetal(std::stringstream& output, TranslationResult& result) {
        std::string entryName = m_metadata.entryPoint;
        
        switch (m_shaderType) {
            case ShaderType::VERTEX: {
                output << "vertex VertexOut " << entryName << "(VertexIn in [[stage_in]],\n";
                output << "                                   constant Uniforms& uniforms [[buffer(0)]]) {\n";
                output << "    VertexOut out;\n";
                output << "    out.position = float4(in.position, 1.0);\n";
                output << "    return out;\n";
                output << "}\n";
                break;
            }
            case ShaderType::FRAGMENT: {
                output << "fragment float4 " << entryName << "(VertexOut in [[stage_in]],\n";
                output << "                                     constant Uniforms& uniforms [[buffer(0)]]) {\n";
                output << "    return float4(1.0, 1.0, 1.0, 1.0);\n";
                output << "}\n";
                break;
            }
            case ShaderType::COMPUTE: {
                output << "kernel void " << entryName << "(uint3 id [[thread_position_in_grid]]) {\n";
                output << "    // Compute kernel body\n";
                output << "}\n";
                break;
            }
            default:
                output << "void " << entryName << "() {\n}\n";
                break;
        }
    }

    void TranslateToGLSL(TranslationResult& result) {
        std::stringstream output;
        
        output << "#version 450 core\n\n";
        output << "// Auto-generated by ArenaX3 Shader Translator\n\n";

        TranslateInputsToGLSL(output);
        TranslateOutputsToGLSL(output);
        TranslateUniformsToGLSL(output);
        TranslateFunctionsToGLSL(output);

        result.code = output.str();
    }

    void TranslateInputsToGLSL(std::stringstream& output) {
        for (size_t i = 0; i < m_metadata.inputs.size(); ++i) {
            const auto& var = m_metadata.inputs[i];
            output << "layout(location = " << i << ") in " << var.type << " " << var.name << ";\n";
        }
        if (!m_metadata.inputs.empty()) output << "\n";
    }

    void TranslateOutputsToGLSL(std::stringstream& output) {
        for (size_t i = 0; i < m_metadata.outputs.size(); ++i) {
            const auto& var = m_metadata.outputs[i];
            output << "layout(location = " << i << ") out " << var.type << " " << var.name << ";\n";
        }
        if (!m_metadata.outputs.empty()) output << "\n";
    }

    void TranslateUniformsToGLSL(std::stringstream& output) {
        for (const auto& var : m_metadata.uniforms) {
            output << "uniform " << var.type << " " << var.name << ";\n";
        }
        if (!m_metadata.uniforms.empty()) output << "\n";
    }

    void TranslateFunctionsToGLSL(std::stringstream& output) {
        output << "void main() {\n";
        output << "    // Main shader body\n";
        output << "}\n";
    }

    void TranslateToESSLImpl(TranslationResult& result) {
        std::stringstream output;
        
        output << "#version 300 es\n";
        output << "precision highp float;\n";
        output << "precision highp int;\n\n";
        output << "// Auto-generated by ArenaX3 Shader Translator\n";
        output << "// Target: OpenGL ES 3.0\n\n";

        TranslateInputsToGLSL(output);
        TranslateOutputsToGLSL(output);
        TranslateUniformsToGLSL(output);
        TranslateFunctionsToGLSL(output);

        result.code = output.str();
    }

    void TranslateToSPIRV(TranslationResult& result) {
        result.warnings.push_back("SPIR-V translation requires external tool (glslangValidator)");
        result.code = m_currentSource;
    }

    std::string MapTypeToHLSL(const std::string& glslType) {
        auto it = glslToHlslTypeMap.find(glslType);
        if (it != glslToHlslTypeMap.end()) {
            return it->second;
        }

        if (glslType.find("vec") == 0) {
            if (glslType == "vec2") return "float2";
            if (glslType == "vec3") return "float3";
            if (glslType == "vec4") return "float4";
        }
        if (glslType.find("ivec") == 0) {
            if (glslType == "ivec2") return "int2";
            if (glslType == "ivec3") return "int3";
            if (glslType == "ivec4") return "int4";
        }
        if (glslType.find("uvec") == 0) {
            if (glslType == "uvec2") return "uint2";
            if (glslType == "uvec3") return "uint3";
            if (glslType == "uvec4") return "uint4";
        }
        if (glslType.find("mat") == 0) {
            if (glslType == "mat2") return "float2x2";
            if (glslType == "mat3") return "float3x3";
            if (glslType == "mat4") return "float4x4";
        }

        return glslType;
    }

    std::string MapTypeToMetal(const std::string& glslType) {
        auto it = glslToMetalTypeMap.find(glslType);
        if (it != glslToMetalTypeMap.end()) {
            return it->second;
        }

        if (glslType.find("vec") == 0) {
            if (glslType == "vec2") return "float2";
            if (glslType == "vec3") return "float3";
            if (glslType == "vec4") return "float4";
        }
        if (glslType.find("ivec") == 0) {
            if (glslType == "ivec2") return "int2";
            if (glslType == "ivec3") return "int3";
            if (glslType == "ivec4") return "int4";
        }
        if (glslType.find("mat") == 0) {
            if (glslType == "mat2") return "float2x2";
            if (glslType == "mat3") return "float3x3";
            if (glslType == "mat4") return "float4x4";
        }

        return glslType;
    }

    std::string MapTextureTypeToHLSL(const std::string& glslType) {
        if (glslType.find("texture2D") != std::string::npos) return "Texture2D";
        if (glslType.find("texture3D") != std::string::npos) return "Texture3D";
        if (glslType.find("textureCube") != std::string::npos) return "TextureCube";
        if (glslType.find("texture2DMS") != std::string::npos) return "Texture2DMS";
        return "Texture2D";
    }

    std::string GetHLSLInputSemantic(const std::string& name, size_t index) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == "position" || lower == "vertex" || lower == "pos") {
            return "POSITION";
        }
        if (lower == "normal" || lower == "n") {
            return "NORMAL";
        }
        if (lower.find("texcoord") != std::string::npos || lower.find("uv") != std::string::npos) {
            return "TEXCOORD" + std::to_string(index);
        }
        if (lower.find("color") != std::string::npos || lower.find("colour") != std::string::npos) {
            return "COLOR" + std::to_string(index);
        }
        if (lower.find("tangent") != std::string::npos) {
            return "TANGENT";
        }
        if (lower.find("binormal") != std::string::npos || lower.find("bitangent") != std::string::npos) {
            return "BINORMAL";
        }
        
        return "TEXCOORD" + std::to_string(index);
    }

    std::string GetHLSLOutputSemantic(const std::string& name, size_t index) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == "position" || lower == "pos") {
            return "SV_POSITION";
        }
        if (lower.find("texcoord") != std::string::npos || lower.find("uv") != std::string::npos) {
            return "TEXCOORD" + std::to_string(index);
        }
        if (lower.find("color") != std::string::npos || lower.find("colour") != std::string::npos) {
            return "COLOR" + std::to_string(index);
        }
        
        return "TEXCOORD" + std::to_string(index);
    }

    std::string GetMetalAttributeName(const std::string& name, size_t index) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower.find("position") != std::string::npos) {
            return "[[attribute(" + std::to_string(index) + ")]]";
        }
        return "[[attribute(" + std::to_string(index) + ")]]";
    }

    static std::string TrimString(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != std::string::npos) {
            str.replace(startPos, from.length(), to);
            startPos += to.length();
        }
        return str;
    }
};

const std::unordered_map<std::string, std::string> Shader_Translator::glslToHlslTypeMap = {
    {"float", "float"},
    {"int", "int"},
    {"uint", "uint"},
    {"bool", "bool"},
    {"double", "double"},
    {"vec2", "float2"},
    {"vec3", "float3"},
    {"vec4", "float4"},
    {"ivec2", "int2"},
    {"ivec3", "int3"},
    {"ivec4", "int4"},
    {"uvec2", "uint2"},
    {"uvec3", "uint3"},
    {"uvec4", "uint4"},
    {"bvec2", "bool2"},
    {"bvec3", "bool3"},
    {"bvec4", "bool4"},
    {"mat2", "float2x2"},
    {"mat3", "float3x3"},
    {"mat4", "float4x4"},
    {"mat2x2", "float2x2"},
    {"mat2x3", "float2x3"},
    {"mat2x4", "float2x4"},
    {"mat3x2", "float3x2"},
    {"mat3x3", "float3x3"},
    {"mat3x4", "float3x4"},
    {"mat4x2", "float4x2"},
    {"mat4x3", "float4x3"},
    {"mat4x4", "float4x4"},
    {"sampler2D", "Texture2D"},
    {"sampler3D", "Texture3D"},
    {"samplerCube", "TextureCube"},
    {"void", "void"}
};

const std::unordered_map<std::string, std::string> Shader_Translator::glslToMetalTypeMap = {
    {"float", "float"},
    {"int", "int"},
    {"uint", "uint"},
    {"bool", "bool"},
    {"vec2", "float2"},
    {"vec3", "float3"},
    {"vec4", "float4"},
    {"ivec2", "int2"},
    {"ivec3", "int3"},
    {"ivec4", "int4"},
    {"uvec2", "uint2"},
    {"uvec3", "uint3"},
    {"uvec4", "uint4"},
    {"mat2", "float2x2"},
    {"mat3", "float3x3"},
    {"mat4", "float4x4"},
    {"mat2x2", "float2x2"},
    {"mat2x3", "float2x3"},
    {"mat2x4", "float2x4"},
    {"mat3x2", "float3x2"},
    {"mat3x3", "float3x3"},
    {"mat3x4", "float3x4"},
    {"mat4x2", "float4x2"},
    {"mat4x3", "float4x3"},
    {"mat4x4", "float4x4"},
    {"texture2d<float>", "texture2d<float>"},
    {"texture3d<float>", "texture3d<float>"},
    {"texturecube<float>", "texturecube<float>"},
    {"sampler", "sampler"},
    {"void", "void"}
};

const std::unordered_map<std::string, std::string> Shader_Translator::hlslToGlslTypeMap = {
    {"float", "float"},
    {"float2", "vec2"},
    {"float3", "vec3"},
    {"float4", "vec4"},
    {"int", "int"},
    {"int2", "ivec2"},
    {"int3", "ivec3"},
    {"int4", "ivec4"},
    {"uint", "uint"},
    {"uint2", "uvec2"},
    {"uint3", "uvec3"},
    {"uint4", "uvec4"},
    {"bool", "bool"},
    {"float2x2", "mat2"},
    {"float3x3", "mat3"},
    {"float4x4", "mat4"},
    {"Texture2D", "sampler2D"},
    {"Texture3D", "sampler3D"},
    {"TextureCube", "samplerCube"}
};

const std::unordered_map<std::string, std::string> Shader_Translator::intrinsicGlslToHlsl = {
    {"texture", "Sample"},
    {"textureLod", "SampleLevel"},
    {"textureGrad", "SampleGrad"},
    {"textureSize", "GetDimensions"},
    {"fract", "frac"},
    {"mix", "lerp"},
    {"mod", "fmod"},
    {"inversesqrt", "rsqrt"},
    {"dFdx", "ddx"},
    {"dFdy", "ddy"},
    {"fwidth", "fwidth"}
};

const std::unordered_map<std::string, std::string> Shader_Translator::intrinsicGlslToMetal = {
    {"texture", "sample"},
    {"textureLod", "sample"},
    {"fract", "fract"},
    {"mix", "mix"},
    {"mod", "fmod"},
    {"dFdx", "dfdx"},
    {"dFdy", "dfdy"},
    {"fwidth", "fwidth"}
};

const std::vector<std::string> Shader_Translator::glslPrecisionQualifiers = {
    "highp",
    "mediump",
    "lowp"
};

} // namespace arenax3
} // namespace com