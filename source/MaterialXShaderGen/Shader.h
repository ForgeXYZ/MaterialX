#ifndef MATERIALX_SHADER_H
#define MATERIALX_SHADER_H

#include <MaterialXCore/Library.h>
#include <MaterialXCore/Node.h>
#include <MaterialXShaderGen/SgNode.h>

#include <queue>
#include <sstream>
#include <unordered_map>

/// Macro for being/end of statements to be picked up by a given shader stage.
/// For shaders that are multi-stage all code generation statements adding code 
/// to the shader should be wrapped inside such begin/end stating its target.
#define BEGIN_SHADER_STAGE(shader, stageId) if (shader.getActiveStage() == stageId) {
#define END_SHADER_STAGE(shader, stageId) }

namespace MaterialX
{

using ShaderPtr = shared_ptr<class Shader>;

/// Class containing all data needed during shader generation.
/// After generation is completed it will contain the resulting source code
/// emitted by shader generators.
///
/// The class contains a default implementation using a single shader stage.
/// Derived shaders can override this, as well as overriding all methods 
/// that add code to the shader.
///
class Shader
{
public:
    /// Bracket types
    enum class Brackets
    {
        NONE,
        BRACES,
        PARENTHESES,
        SQUARES
    };

    /// 
    enum class VDirection
    {
        UP,
        DOWN
    };

    struct Variable
    {
        string type;
        string name;
        string semantic;
        ValuePtr value;

        Variable(const string& t = EMPTY_STRING, const string& n = EMPTY_STRING, const string& s = EMPTY_STRING, ValuePtr v = nullptr)
            : type(t)
            , name(n)
            , semantic(s)
            , value(v)
        {}
    };

    using VariablePtr = std::shared_ptr<Variable>;

    /// A block of variables for a shader stage
    struct VariableBlock
    {
        string name;
        string instance;
        std::unordered_map<string, VariablePtr> variableMap;
        std::vector<Variable*> variableOrder;
        VariableBlock(const string& n, const string& i) : name(n), instance(i) {}
    };

    using VariableBlockPtr = std::shared_ptr<VariableBlock>;
    using VariableBlockMap = std::unordered_map<string, VariableBlockPtr>;

    /// Identifier for shader stages. The base class shader has only a single 
    /// pixel shader stage. Derived shader classes can define additional stages.
    static const size_t PIXEL_STAGE = 0;
    static const size_t NUM_STAGES  = 1;

    /// Identifier for variable blocks. Derived classes can define additional blocks.
    static const string GLOBAL_SCOPE;
    static const string SHADER_INTERFACE;

public:
    /// Constructor
    Shader(const string& name);

    /// Destructor
    virtual ~Shader() {}

    /// Initialize the shader before shader generation.
    /// @param element The root element to generate the shader from. 
    /// @param shadergen The shader generator instance.
    virtual void initialize(ElementPtr element, ShaderGenerator& shadergen);

    /// Return the number of shader stages for this shader.
    /// Defaults to a single stage, derived classes can override this.
    virtual size_t numStages() const { return NUM_STAGES; }

    /// Set the active stage that code will be added to
    virtual void setActiveStage(size_t stage) { _activeStage = stage; }

    /// Return the active stage
    virtual size_t getActiveStage() const { return _activeStage; }

    /// Create a new variable block for uniform inputs.
    virtual void createUniformBlock(const string& block, const string& instance = EMPTY_STRING);

    /// Create a new variable for uniform data in the given block. 
    /// The block must be previously created with createUniformBlock.
    virtual void createUniform(const string& block, const string& type, const string& name, 
        const string& sementic = EMPTY_STRING, ValuePtr value = nullptr);

    /// Create a new variable for application/geometric data (primvars).
    virtual void createAppData(const string& type, const string& name, const string& sementic = EMPTY_STRING);

    /// Return all blocks of uniform variables.
    const VariableBlockMap& getUniformBlocks() { return _uniforms; }

    /// Return a specific block of uniform variables.
    const VariableBlock& getUniformBlock(const string& block);

    /// Return the block of application data variables.
    const VariableBlock& getAppDataBlock() { return _appData; }

    /// Start a new scope in the shader, using the given bracket type
    virtual void beginScope(Brackets brackets = Brackets::BRACES);

    /// End the current scope in the shader
    virtual void endScope(bool semicolon = false, bool newline = true);

    /// Start a new line in the shader
    virtual void beginLine();

    /// End the current line in the shader
    virtual void endLine(bool semicolon = true);

    /// Add a newline character to the shader
    virtual void newLine();

    /// Add a string to the shader
    virtual void addStr(const string& str);

    /// Add a single line of code to the shader,
    /// optionally appening a semi-colon
    virtual void addLine(const string& str, bool semicolon = true);

    /// Add a block of code to the shader
    virtual void addBlock(const string& str);

    /// Add the function definition for a node
    virtual void addFunctionDefinition(SgNode* node, ShaderGenerator& shadergen);

    /// Add the function call for a node
    virtual void addFunctionCall(SgNode* node, ShaderGenerator& shadergen);

    /// Add the contents of an include file
    /// Making sure it is only included once
    /// for a shader stage
    virtual void addInclude(const string& file, ShaderGenerator& shadergen);

    /// Add a single line of code comment to the shader
    virtual void addComment(const string& str);

    /// Add a value to the shader
    template<typename T>
    void addValue(const T& value)
    {
        std::stringstream str;
        str << value;
        stage().code += str.str();
    }

    /// Return the shader name
    const string& getName() const { return _name; }

    /// Return the active shader graph.
    SgNodeGraph* getNodeGraph() const { return _graphStack.back(); }

    /// Push a new active shader graph.
    /// Used when emitting code for compounds / subgraphs.
    void pushActiveGraph(SgNodeGraph* graph) { _graphStack.push_back(graph); }

    /// Reactivate the previously last used shader graph.
    void popActiveGraph() { _graphStack.pop_back(); }

    /// Return true if this shader matches the given classification.
    bool hasClassification(unsigned int c) const { return _rootGraph->hasClassification(c); }

    /// Return the vdirection requested in the current document.
    VDirection getRequestedVDirection() const { return _vdirection; }

    /// Return the final shader source code for a given shader stage
    const string& getSourceCode(size_t stage = PIXEL_STAGE) const { return _stages[stage].code; }

    bool isTransparent() const
    {
        if (getNodeGraph()->hasClassification(SgNode::Classification::SHADER))
        {
            for (SgNode* node : getNodeGraph()->getNodes())
            {
                if (node && node->hasClassification(SgNode::Classification::SHADER))
                {
                    MaterialX::SgImplementation* implementation = node->getImplementation();
                    if (implementation)
                    {
                        return implementation->isTransparent(*node);
                    }
                }
            }
        }
        return false;
    }

protected:

    /// A shader stage, containing the state and 
    /// resulting source code for the stage
    struct Stage
    {
        int indentations;
        std::queue<Brackets> scopes;
        std::set<string> includes;
        std::set<SgImplementation*> definedFunctions;
        string code;

        Stage() : indentations(0) {}
    };

    /// Return the currently active stage
    Stage& stage() { return _stages[_activeStage]; }

    /// Add indentation on current line
    virtual void indent();

    string _name;
    SgNodeGraphPtr _rootGraph;
    vector<SgNodeGraph*> _graphStack;
    VDirection _vdirection;

    size_t _activeStage;
    vector<Stage> _stages;

    // Block holding application/geometric input variables
    VariableBlock _appData;

    // Map of blocks holding uniform input variables
    VariableBlockMap _uniforms;
};

/// @class @ExceptionShaderGenError
/// An exception that is thrown when shader generation fails.
class ExceptionShaderGenError : public Exception
{
public:
    ExceptionShaderGenError(const string& msg) :
        Exception(msg)
    {
    }

    ExceptionShaderGenError(const ExceptionShaderGenError& e) :
        Exception(e)
    {
    }

    virtual ~ExceptionShaderGenError() throw()
    {
    }
};

} // namespace MaterialX

#endif
