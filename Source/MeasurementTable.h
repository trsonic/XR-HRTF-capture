#pragma once
#include <JuceHeader.h>

class MeasurementTable : public juce::Component,
                                public juce::TableListBoxModel
{
public:
    MeasurementTable()
    {
        loadData();                                                                  // [1]

        addAndMakeVisible(table);                                                  // [1]

        table.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        table.setOutlineThickness(1);

        if (columnList != nullptr)
        {
            forEachXmlChildElement(*columnList, columnXml)
            {
                table.getHeader().addColumn(columnXml->getStringAttribute("name"), // [2]
                    columnXml->getIntAttribute("columnId"),
                    columnXml->getIntAttribute("width"),
                    50,
                    400,
                    juce::TableHeaderComponent::defaultFlags);
            }
        }

        table.getHeader().setSortColumnId(1, true);                                // [3]
        table.setMultipleSelectionEnabled(false);                                   // [4]

    }

    int getNumRows() override
    {
        return numRows;
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        auto alternateColour = getLookAndFeel().findColour(juce::ListBox::backgroundColourId)
            .interpolatedWith(getLookAndFeel().findColour(juce::ListBox::textColourId), 0.03f);
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);
        else if (rowNumber % 2)
            g.fillAll(alternateColour);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId,
        int width, int height, bool rowIsSelected) override
    {
        g.setColour(rowIsSelected ? juce::Colours::darkblue : getLookAndFeel().findColour(juce::ListBox::textColourId));  // [5]
        g.setFont(font);

        if (auto* rowElement = dataList->getChildElement(rowNumber))
        {
            auto text = rowElement->getStringAttribute(getAttributeNameForColumnId(columnId));

            g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);                             // [6]
        }

        g.setColour(getLookAndFeel().findColour(juce::ListBox::backgroundColourId));
        g.fillRect(width - 1, 0, 1, height);                                                                               // [7]
    }

    int getColumnAutoSizeWidth(int columnId) override
    {
        //if (columnId == 9)
        //    return 50;

        int widest = 32;

        for (auto i = getNumRows(); --i >= 0;)
        {
            if (auto* rowElement = dataList->getChildElement(i))
            {
                auto text = rowElement->getStringAttribute(getAttributeNameForColumnId(columnId));

                widest = juce::jmax(widest, font.getStringWidth(text));
            }
        }

        return widest + 8;
    }

    //int getSelection(const int rowNumber) const
    //{
    //    return dataList->getChildElement(rowNumber)->getIntAttribute("Select");
    //}

    //void setSelection(const int rowNumber, const int newSelection)
    //{
    //    dataList->getChildElement(rowNumber)->setAttribute("Select", newSelection);
    //}

    juce::String getText(const int columnNumber, const int rowNumber) const
    {
        return dataList->getChildElement(rowNumber)->getStringAttribute(getAttributeNameForColumnId(columnNumber));
    }

    void setText(const int columnNumber, const int rowNumber, const juce::String& newText)
    {
        const auto& columnName = table.getHeader().getColumnName(columnNumber);
        dataList->getChildElement(rowNumber)->setAttribute(columnName, newText);
    }

    //==============================================================================
    void resized() override
    {
        table.setBoundsInset(juce::BorderSize<int>(8));
    }

    void selectMeasurementRow(const int id)
    {
        if (id == 0)
            table.deselectAllRows();
        else if (id >= 1)
            table.selectRow(id - 1);
        else
            return;
    }

    String getFromXML(int id, String column)
    {
        if (id >= 1 && id <= dataList->getNumChildElements())
        {
            int colId = columnList->getChildByAttribute("name", column)->getAttributeValue(0).getIntValue();
            return dataList->getChildByAttribute("ID", String(id).formatted("%02d"))->getAttributeValue(colId - 1);
        }
        else
        {
            return "";
        }
    }

private:
    juce::TableListBox table{ {}, this };
    juce::Font font{ 14.0f };

    std::unique_ptr<juce::XmlElement> tutorialData;
    juce::XmlElement* columnList = nullptr;
    juce::XmlElement* dataList = nullptr;
    int numRows = 0;

    void loadData()
    {
        auto dir = juce::File::getCurrentWorkingDirectory();

        int numTries = 0;

        while (!dir.getChildFile("Resources").exists() && numTries++ < 15)
            dir = dir.getParentDirectory();

        auto tableFile = dir.getChildFile("Resources").getChildFile("speaker_angles.xml");

        if (tableFile.exists())
        {
            tutorialData = juce::XmlDocument::parse(tableFile);            // [3]

            dataList = tutorialData->getChildByName("DATA");
            columnList = tutorialData->getChildByName("HEADERS");          // [4]

            numRows = dataList->getNumChildElements();                      // [5]
        }
    }

    juce::String getAttributeNameForColumnId(const int columnId) const
    {
        forEachXmlChildElement(*columnList, columnXml)
        {
            if (columnXml->getIntAttribute("columnId") == columnId)
                return columnXml->getStringAttribute("name");
        }

        return {};
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeasurementTable)
};
