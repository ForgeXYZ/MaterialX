#include <MaterialXView/Editor.h>

#include <MaterialXView/Viewer.h>

#include <nanogui/button.h>
#include <nanogui/combobox.h>
#include <nanogui/layout.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/textbox.h>

namespace {

class EditorFormHelper : public ng::FormHelper
{
  public:
    explicit EditorFormHelper(ng::Screen *screen) : ng::FormHelper(screen) { }
    ~EditorFormHelper() { }

    void setPreGroupSpacing(int val) { mPreGroupSpacing = val; }
    void setPostGroupSpacing(int val) { mPostGroupSpacing = val; }
    void setVariableSpacing(int val) { mVariableSpacing = val; }
};

} // anonymous namespace

//
// PropertyEditor methods
//

PropertyEditor::PropertyEditor() :
    _visible(false),
    _container(nullptr),
    _formWindow(nullptr),
    _fileDialogsForImages(true)
{
}

void PropertyEditor::create(Viewer& parent)
{
    ng::Window* parentWindow = parent.getWindow();

    // Remove the window associated with the form.
    // This is done by explicitly creating and owning the window
    // as opposed to having it being done by the form
    ng::Vector2i previousPosition(15, parentWindow->height() + 60);
    if (_formWindow)
    {
        for (int i = 0; i < _formWindow->childCount(); i++)
        {
            _formWindow->removeChild(i);
        }
        // We don't want the property editor to move when
        // we update it's contents so cache any previous position
        // to use when we create a new window.
        previousPosition = _formWindow->position();
        parent.removeChild(_formWindow);
    }

    if (previousPosition.x() < 0)
        previousPosition.x() = 0;
    if (previousPosition.y() < 0)
        previousPosition.y() = 0;

    _formWindow = new ng::Window(&parent, "Property Editor");
    _formWindow->setLayout(new ng::GroupLayout());
    _formWindow->setPosition(previousPosition);
    _formWindow->setVisible(_visible);

    ng::VScrollPanel *scroll_panel = new ng::VScrollPanel(_formWindow);
    scroll_panel->setFixedHeight(200);
    _container = new ng::Widget(scroll_panel);

    ng::GridLayout *layout = new ng::GridLayout(ng::Orientation::Horizontal, 2,
                                                ng::Alignment::Minimum, 2, 2);
    layout->setColAlignment({ ng::Alignment::Minimum, ng::Alignment::Minimum });
    _container->setLayout(layout);
}

void PropertyEditor::addItemToForm(const mx::UIPropertyItem& item, const std::string& group,
                                   ng::Widget* container, Viewer* viewer, bool editable)
{
    const mx::UIProperties& ui = item.ui;
    mx::ValuePtr value = item.variable->getValue();
    const std::string& label = item.label;
    const std::string& path = item.variable->getPath();
    mx::ValuePtr min = ui.uiMin;
    mx::ValuePtr max = ui.uiMax;
    const mx::StringVec& enumeration = ui.enumeration;
    const std::vector<mx::ValuePtr> enumValues = ui.enumerationValues;

    if (!value)
    {
        return;
    }

    if (!group.empty())
    {
        ng::Label* groupLabel =  new ng::Label(container, group);
        groupLabel->setFontSize(20);
        groupLabel->setFont("sans-bold");
        new ng::Label(container, "");
        new ng::Label(container, "");
        new ng::Label(container, "");
    }

    // Integer input. Can map to a combo box if an enumeration
    if (value->isA<int>())
    {
        int v = value->asA<int>();

        // Create a combo box. The items are the enumerations in order.
        if (v < (int) enumeration.size())
        {
            std::string enumValue = enumeration[v];

            new ng::Label(container, label);
            ng::ComboBox* comboBox = new ng::ComboBox(container, {""});
            comboBox->setEnabled(editable);
            comboBox->setItems(enumeration);
            comboBox->setSelectedIndex(v);
            comboBox->setFixedSize(ng::Vector2i(100, 20));
            comboBox->setFontSize(15);
            comboBox->setCallback([path, viewer, enumValues](int v)
            {
                MaterialPtr material = viewer->getSelectedMaterial();
                mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                if (uniform)
                {
                    material->getShader()->bind();
                    if (v < (int) enumValues.size())
                    {
                        material->getShader()->setUniform(uniform->getName(), enumValues[v]->asA<int>());
                    }
                    else
                    {
                        material->getShader()->setUniform(uniform->getName(), v);
                    }
                }
            });
        }
        else
        {
            new ng::Label(container, label);
            auto intVar = new ng::IntBox<int>(container);
            intVar->setFixedSize(ng::Vector2i(100, 20));
            intVar->setFontSize(15);
            intVar->setSpinnable(editable);
            intVar->setCallback([path, viewer](int v)
            {
                MaterialPtr material = viewer->getSelectedMaterial();
                if (material)
                {
                    mx::ShaderPort* uniform = material->findUniform(path);
                    if (uniform)
                    {
                        material->getShader()->bind();
                        material->getShader()->setUniform(uniform->getName(), v);
                    }
                }
            });
        }
    }

    // Float widget
    else if (value->isA<float>())
    {
        new ng::Label(container, label);
        float v = value->asA<float>();
        auto floatVar = new ng::FloatBox<float>(container, v);
        floatVar->setFixedSize(ng::Vector2i(100, 20));
        floatVar->setSpinnable(editable);
        floatVar->setEditable(editable);
        floatVar->setFontSize(15);
        if (min)
            floatVar->setMinValue(min->asA<float>());
        if (max)
            floatVar->setMaxValue(max->asA<float>());
        floatVar->setCallback([path, viewer](float v)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            if (material)
            {
                mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                if (uniform)
                {
                    material->getShader()->bind();
                    material->getShader()->setUniform(uniform->getName(), v);
                }                
            }
        });
    }

    // Boolean widget
    else if (value->isA<bool>())
    {
        bool v = value->asA<bool>();
        new ng::Label(container, label);
        ng::CheckBox* boolVar = new ng::CheckBox(container, "");
        boolVar->setChecked(v);
        boolVar->setFontSize(15);
        boolVar->setCallback([path, viewer](bool v)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
    }

    // Color2 input
    else if (value->isA<mx::Color2>())
    {
        new ng::Label(container, label);
        mx::Color2 v = value->asA<mx::Color2>();
        ng::Color c;
        c.r() = v[0];
        c.g() = v[1];
        c.b() = 0.0f;
        c.w() = 1.0f;
        auto colorVar = new ng::ColorPicker(container, c);
        colorVar->setFixedSize({ 100, 20 });
        colorVar->setFontSize(15);
        colorVar->setFinalCallback([path, viewer, colorVar](const ng::Color &c)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector2f v;
                v.x() = c.r();
                v.y() = c.g();
                material->getShader()->setUniform(uniform->getName(), v);
                ng::Color c2 = c;
                c2.b() = 0.0f;
                c2.w() = 1.0f;
                colorVar->setColor(c2);
            }
        });
    }

    // Color3 input. Can map to a combo box if an enumeration
    else if (value->isA<mx::Color3>())
    {
        // Determine if there is an enumeration for this
        mx::Color3 color = value->asA<mx::Color3>();
        int index = -1;
        if (enumeration.size() && enumValues.size())
        {
            index = 0;
            for (size_t i = 0; i < enumValues.size(); i++)
            {
                if (enumValues[i]->asA<mx::Color3>() == color)
                {
                    index = static_cast<int>(i);
                    break;
                }
            }
        }

        // Create a combo box. The items are the enumerations in order.
        if (index >= 0)
        {
            ng::ComboBox* comboBox = new ng::ComboBox(container, { "" });
            comboBox->setEnabled(editable);
            comboBox->setItems(enumeration);
            comboBox->setSelectedIndex(index);
            comboBox->setFontSize(15);
            comboBox->setCallback([path, enumValues, viewer](int index)
            {
                MaterialPtr material = viewer->getSelectedMaterial();
                mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                if (uniform)
                {
                    material->getShader()->bind();
                    if (index < (int) enumValues.size())
                    {
                        mx::Color3 c = enumValues[index]->asA<mx::Color3>();
                        ng::Vector3f v;
                        v.x() = c[0];
                        v.y() = c[1];
                        v.z() = c[2];
                        material->getShader()->setUniform(uniform->getName(), v);
                    }
                }
            });
        }
        else
        {
            mx::Color3 v = value->asA<mx::Color3>();
            ng::Color c;
            c.r() = v[0];
            c.g() = v[1];
            c.b() = v[2];
            c.w() = 1.0;
            
            new ng::Label(container, label);
            auto colorVar = new ng::ColorPicker(container, c);
            colorVar->setFixedSize({ 100, 20 });
            colorVar->setFontSize(15);
            colorVar->setFinalCallback([path, viewer](const ng::Color &c)
            {
                MaterialPtr material = viewer->getSelectedMaterial();
                mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                if (uniform)
                {
                    material->getShader()->bind();
                    ng::Vector3f v;
                    v.x() = c.r();
                    v.y() = c.g();
                    v.z() = c.b();
                    material->getShader()->setUniform(uniform->getName(), v);
                }
            });
        }
    }

    // Color4 input
    else if (value->isA<mx::Color4>())
    {
        new ng::Label(container, label);
        mx::Color4 v = value->asA<mx::Color4>();
        ng::Color c;
        c.r() = v[0];
        c.g() = v[1];
        c.b() = v[2];
        c.w() = v[3];
        auto colorVar = new ng::ColorPicker(container, c);
        colorVar->setFixedSize({ 100, 20 });
        colorVar->setFontSize(15);
        colorVar->setFinalCallback([path, viewer](const ng::Color &c)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector4f v;
                v.x() = c.r();
                v.y() = c.g();
                v.z() = c.b();
                v.w() = c.w();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
    }

    // Vec 2 widget
    else if (value->isA<mx::Vector2>())
    {
        mx::Vector2 v = value->asA<mx::Vector2>();
        new ng::Label(container, label + ".x");
        auto v1 = new ng::FloatBox<float>(container, v[0]);
        v1->setFixedSize({ 100, 20 });
        v1->setFontSize(15);
        new ng::Label(container, label + ".y");
        auto v2 = new ng::FloatBox<float>(container, v[1]);
        v2->setFixedSize({ 100, 20 });
        v2->setFontSize(15);
        v1->setCallback([v2, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector2f v;
                v.x() = f;
                v.y() = v2->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v1->setSpinnable(editable);
        v2->setCallback([v1, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector2f v;
                v.x() = v1->value();
                v.y() = f;
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v2->setSpinnable(editable);
    }

    // Vec 3 input
    else if (value->isA<mx::Vector3>())
    {
        mx::Vector3 v = value->asA<mx::Vector3>();
        new ng::Label(container, label + ".x");
        auto v1 = new ng::FloatBox<float>(container, v[0]);
        v1->setFixedSize({ 100, 20 });
        v1->setFontSize(15);
        new ng::Label(container, label + ".y");
        auto v2 = new ng::FloatBox<float>(container, v[1]);
        v2->setFixedSize({ 100, 20 });
        v2->setFontSize(15);
        new ng::Label(container, label + ".z");
        auto v3 = new ng::FloatBox<float>(container, v[2]);
        v3->setFixedSize({ 100, 20 });
        v3->setFontSize(15);

        v1->setCallback([v2, v3, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector3f v;
                v.x() = f;
                v.y() = v2->value();
                v.z() = v3->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v1->setSpinnable(editable);
        v2->setCallback([v1, v3, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector3f v;
                v.x() = v1->value();
                v.y() = f;
                v.z() = v3->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v2->setSpinnable(editable);
        v3->setCallback([v1, v2, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector3f v;
                v.x() = v1->value();
                v.y() = v2->value();
                v.z() = f;
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v3->setSpinnable(editable);
    }

    // Vec 4 input
    else if (value->isA<mx::Vector4>())
    {
        mx::Vector4 v = value->asA<mx::Vector4>();
        new ng::Label(container, label + ".x");
        auto v1 = new ng::FloatBox<float>(container, v[0]);
        v1->setFixedSize({ 100, 20 });
        v1->setFontSize(15);
        new ng::Label(container, label + ".y");
        auto v2 = new ng::FloatBox<float>(container, v[1]);
        v2->setFixedSize({ 100, 20 });
        v1->setFontSize(15);
        new ng::Label(container, label + ".z");
        auto v3 = new ng::FloatBox<float>(container, v[2]);
        v3->setFixedSize({ 100, 20 });
        v1->setFontSize(15);
        new ng::Label(container, label + ".w");
        auto v4 = new ng::FloatBox<float>(container, v[3]);
        v4->setFixedSize({ 100, 20 });
        v1->setFontSize(15);

        v1->setCallback([v2, v3, v4, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector4f v;
                v.x() = f;
                v.y() = v2->value();
                v.z() = v3->value();
                v.w() = v4->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v1->setSpinnable(editable);
        v2->setCallback([v1, v3, v4, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector4f v;
                v.x() = v1->value();
                v.y() = f;
                v.z() = v3->value();
                v.w() = v4->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v2->setSpinnable(editable);
        v3->setCallback([v1, v2, v4, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector4f v;
                v.x() = v1->value();
                v.y() = v2->value();
                v.z() = f;
                v.w() = v4->value();
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v3->setSpinnable(editable);
        v4->setCallback([v1, v2, v3, path, viewer](float f)
        {
            MaterialPtr material = viewer->getSelectedMaterial();
            mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
            if (uniform)
            {
                material->getShader()->bind();
                ng::Vector4f v;
                v.x() = v1->value();
                v.y() = v2->value();
                v.z() = v3->value();
                v.w() = f;
                material->getShader()->setUniform(uniform->getName(), v);
            }
        });
        v4->setSpinnable(editable);
    }

    // String
    else if (value->isA<std::string>())
    {
        std::string v = value->asA<std::string>();
        if (!v.empty())
        {
            if (item.variable->getType() == MaterialX::Type::FILENAME)
            {
                new ng::Label(container, label);
                ng::Button* buttonVar = new ng::Button(container, mx::FilePath(v).getBaseName());
                buttonVar->setEnabled(editable);
                buttonVar->setFontSize(15);
                buttonVar->setCallback([buttonVar, path, viewer]()
                {
                    MaterialPtr material = viewer->getSelectedMaterial();
                    mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                    if (uniform)
                    {
                        if (uniform->getType() == MaterialX::Type::FILENAME)
                        {
                            const mx::GLTextureHandlerPtr handler = viewer->getImageHandler();
                            if (handler)
                            {
                                mx::StringSet extensions;
                                handler->supportedExtensions(extensions);
                                std::vector<std::pair<std::string, std::string>> filetypes;
                                for (auto extension : extensions)
                                {
                                    filetypes.push_back(std::make_pair(extension, extension));
                                }
                                std::string filename = ng::file_dialog(filetypes, false);
                                if (!filename.empty())
                                {
                                    uniform->setValue(mx::Value::createValue<std::string>(filename));
                                    buttonVar->setCaption(mx::FilePath(filename).getBaseName());
                                    viewer->performLayout();
                                }
                            }
                        }
                    }
                });
            }
            else
            {
                new ng::Label(container, label);
                ng::TextBox* stringVar =  new ng::TextBox(container, v);
                stringVar->setFixedSize({ 100, 20 });
                stringVar->setFontSize(15);
                stringVar->setCallback([path, viewer](const std::string &v)
                {
                    MaterialPtr material = viewer->getSelectedMaterial();
                    mx::ShaderPort* uniform = material ? material->findUniform(path) : nullptr;
                    if (uniform)
                    {
                        uniform->setValue(mx::Value::createValue<std::string>(v));
                    }
                    return true;
                });
            }
        }
    }
}

void PropertyEditor::updateContents(Viewer* viewer)
{
    create(*viewer);

    MaterialPtr material = viewer->getSelectedMaterial();
    mx::TypedElementPtr materialElement = material ? material->getElement() : nullptr;
    if (!materialElement)
    {
        return;
    }

    mx::DocumentPtr contentDocument = viewer->getCurrentDocument();
    if (!contentDocument)
    {
        return;
    }

    bool addedItems = false;
    const MaterialX::VariableBlock* publicUniforms = material->getPublicUniforms();
    if (publicUniforms)
    {
        mx::UIPropertyGroup groups;
        mx::UIPropertyGroup unnamedGroups;
        const std::string pathSeparator(":");
        mx::createUIPropertyGroups(*publicUniforms, contentDocument, materialElement,
                                    pathSeparator, groups, unnamedGroups); 

        std::string previousFolder;
        // Make all inputs editable for now. Could make this read-only as well.
        const bool editable = true;
        for (auto it = groups.begin(); it != groups.end(); ++it)
        {
            const std::string& folder = it->first;
            const mx::UIPropertyItem& item = it->second;

            // Find out if the uniform is editable. Some
            // inputs may be optimized out during compilation.
            if (material->findUniform(item.variable->getPath()))
            {
                addItemToForm(item, (previousFolder == folder) ? mx::EMPTY_STRING : folder, _container, viewer, editable);
                previousFolder.assign(folder);
                addedItems = true;
            }
        }

        if (!unnamedGroups.empty() && groups.empty())
        {
            ng::Label* otherLabel = new ng::Label(_container, "Other");
            otherLabel->setFontSize(20);
            otherLabel->setFont("sans-bold");
            new ng::Label(_container, "");
        }
        for (auto it2 = unnamedGroups.begin(); it2 != unnamedGroups.end(); ++it2)
        {
            const mx::UIPropertyItem& item = it2->second;
            if (material->findUniform(item.variable->getPath()))
            {
                addItemToForm(item, mx::EMPTY_STRING, _container, viewer, editable);
                addedItems = true;
            }
        }
    }
    if (!addedItems)
    {
        new ng::Label(_container, "No Input Parameters");
        new ng::Label(_container, "");
    }

    viewer->performLayout();
}
