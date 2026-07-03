#include "PluginEditor.h"

namespace Colours
{
    const juce::Colour mainBg(0xfff5f7fa);
    const juce::Colour topPanelStart(0xff090e12);
    const juce::Colour topPanelEnd(0xff050709);
    const juce::Colour midBar(0xfff1f3f6);
    const juce::Colour bottomPanel(0xfff7f9fa);
    const juce::Colour border(0xffbbc4cc);
    const juce::Colour accent(0xff2ce5c4);
    const juce::Colour textLabel(0xff718096);
    const juce::Colour textDark(0xff2e3540);
    const juce::Colour toggleStudio(0xff8e99fc);
    const juce::Colour toggleCreative(0xffcbd5e0);
    const juce::Colour autoGainOn(0xff59f38c);
    const juce::Colour autoGainOff(0xffa0aec0);
    const juce::Colour ledGreen(0xff52ec87);
    const juce::Colour ledBlue(0xff4cdff2);
    const juce::Colour ledOrange(0xfffca33d);
    const juce::Colour ledRed(0xffff5e5e);
}

/* ================================================================
   Wireframe Visualizer — More bounce, glow, and gradient bg
   ================================================================ */
class DelayAudioProcessorEditor::WireframeVisualizer : public juce::Component,
                                                       private juce::Timer
{
public:
    WireframeVisualizer() { startTimerHz(60); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        juce::ColourGradient bg(Colours::topPanelStart, 0, 0,
                                Colours::topPanelEnd, 0, b.getHeight(), false);
        g.setGradientFill(bg);
        g.fillRect(b);

        float sx = b.getWidth() / 380.0f;
        float sy = b.getHeight() / 160.0f;
        float s = juce::jmin(sx, sy);
        float ox = (b.getWidth() - 380.0f * s) * 0.5f;
        float oy = (b.getHeight() - 160.0f * s) * 0.5f;

        g.addTransform(juce::AffineTransform::translation(ox, oy).scaled(s, s));

        // More bounce: 4 % scale + 8 px translate + secondary harmonic
        float breath1 = 1.0f + 0.04f * std::sin(phase) + 0.015f * std::sin(phase * 2.7f);
        float breath2 = 1.0f + 0.04f * std::sin(phase + juce::MathConstants<float>::pi)
                        + 0.015f * std::sin((phase + juce::MathConstants<float>::pi) * 2.7f);
        float trans1  = -8.0f * (0.5f + 0.5f * std::sin(phase));
        float trans2  = -8.0f * (0.5f + 0.5f * std::sin(phase + juce::MathConstants<float>::pi));

        float glow1 = 0.5f + 0.5f * std::sin(phase);
        float glow2 = 0.5f + 0.5f * std::sin(phase + juce::MathConstants<float>::pi);

        drawCube(g, {{150,38},{185,58},{150,78},{115,58}},
                    {{150,96},{185,116},{150,136},{115,116}},
                    breath1, trans1, 1.8f, glow1);

        drawCube(g, {{255,54},{276,66},{255,78},{234,66}},
                    {{255,86},{276,98},{255,110},{234,98}},
                    breath2, trans2, 1.5f, glow2);
    }

    void timerCallback() override
    {
        phase += 0.01745f * 1.3f; // slightly faster for more energy
        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;
        repaint();
    }

private:
    float phase = 0.0f;

    void drawCube(juce::Graphics& g,
                  const std::vector<juce::Point<float>>& top,
                  const std::vector<juce::Point<float>>& bot,
                  float breath, float ty, float sw, float glowAlpha)
    {
        juce::Point<float> cen;
        for (auto& p : top) cen += p;
        cen /= (float)top.size();

        auto t = juce::AffineTransform::translation(0, ty)
                             .translated(cen.x, cen.y)
                             .scaled(breath, breath)
                             .translated(-cen.x, -cen.y);

        juce::Path topPath, botPath, all;
        topPath.startNewSubPath(top[0]);
        for (size_t i = 1; i < top.size(); ++i) topPath.lineTo(top[i]);
        topPath.closeSubPath();
        topPath.applyTransform(t);

        botPath.startNewSubPath(bot[0]);
        for (size_t i = 1; i < bot.size(); ++i) botPath.lineTo(bot[i]);
        botPath.closeSubPath();
        botPath.applyTransform(t);

        all.addPath(topPath);
        all.addPath(botPath);

        for (size_t i = 0; i < top.size(); ++i)
        {
            auto a = top[i].transformedBy(t);
            auto c = bot[i].transformedBy(t);
            all.startNewSubPath(a);
            all.lineTo(c);
        }

        // Glow layer
        juce::DropShadow glow(Colours::accent.withAlpha(glowAlpha * 0.7f), 10, {0, 0});
        glow.drawForPath(g, all);

        g.setColour(Colours::accent.withAlpha(0.9f));
        g.strokePath(topPath, juce::PathStrokeType(sw));
        g.strokePath(botPath, juce::PathStrokeType(sw));

        for (size_t i = 0; i < top.size(); ++i)
        {
            auto a = top[i].transformedBy(t);
            auto c = bot[i].transformedBy(t);
            g.drawLine(a.x, a.y, c.x, c.y, sw);
        }
    }
};

/* ================================================================
   Toggle Switch — Studio / Creative
   ================================================================ */
class DelayAudioProcessorEditor::ToggleSwitch : public juce::Component
{
public:
    ToggleSwitch(juce::AudioParameterBool& param) : parameter(param)
    {
        setSize(32, 17);
        attachment = std::make_unique<juce::ParameterAttachment>(
            param, [this](float v) { state = v > 0.5f; repaint(); });
        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(state ? Colours::toggleCreative : Colours::toggleStudio);
        g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);

        float hs = 13.0f;
        float x  = state ? b.getRight() - hs - 2.0f : 2.0f;
        float y  = (b.getHeight() - hs) / 2.0f;

        juce::DropShadow ds(juce::Colours::black.withAlpha(0.2f), 3, {0, 1});
        juce::Path h; h.addEllipse(x, y, hs, hs);
        ds.drawForPath(g, h);

        g.setColour(juce::Colours::white);
        g.fillEllipse(x, y, hs, hs);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        parameter.beginChangeGesture();
        parameter.setValueNotifyingHost(state ? 0.0f : 1.0f);
        parameter.endChangeGesture();
    }

private:
    juce::AudioParameterBool& parameter;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool state = false;
};

/* ================================================================
   Auto Gain Button
   ================================================================ */
class DelayAudioProcessorEditor::AutoGainButton : public juce::Component
{
public:
    AutoGainButton(juce::AudioParameterBool& param) : parameter(param)
    {
        setSize(50, 20);
        attachment = std::make_unique<juce::ParameterAttachment>(
            param, [this](float v) { state = v > 0.5f; repaint(); });
        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff4a5568));
        g.drawText("AUTO", b.removeFromTop(b.getHeight() / 2), juce::Justification::topRight);
        g.drawText("GAIN", b, juce::Justification::topRight);

        auto ind = getLocalBounds().removeFromRight(18).withHeight(11).translated(-2, 4).toFloat();
        g.setColour(state ? Colours::autoGainOn : Colours::autoGainOff);
        g.fillRoundedRectangle(ind, 2.0f);

        if (state)
        {
            juce::DropShadow glow(Colours::autoGainOn.withAlpha(0.75f), 6, {0, 0});
            juce::Path p; p.addRectangle(ind);
            glow.drawForPath(g, p);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawRoundedRectangle(ind.reduced(0.5f), 2.0f, 1.0f);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.1f));
            g.drawRoundedRectangle(ind.reduced(0.5f), 2.0f, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        parameter.beginChangeGesture();
        parameter.setValueNotifyingHost(state ? 0.0f : 1.0f);
        parameter.endChangeGesture();
    }

private:
    juce::AudioParameterBool& parameter;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool state = true;
};

/* ================================================================
   Tempo Ring Control
   ================================================================ */
class DelayAudioProcessorEditor::TempoControl : public juce::Component
{
public:
    TempoControl(juce::AudioParameterChoice& param) : parameter(param)
    {
        setSize(66, 66);
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        attachment = std::make_unique<juce::ParameterAttachment>(
            param, [this](float v) { value = (int)std::round(v * 5.0f); repaint(); });
        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(2);
        auto c = b.getCentre();
        // Exact scaled radius: 28 * (66/70) ≈ 26.4
        float r = 26.4f;

        g.setColour(juce::Colour(0xffe2e8f0));
        g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, 3.5f);

        float prog = (value + 1) / 6.0f;
        float sa = -juce::MathConstants<float>::pi / 2.0f;
        float ea = sa + prog * juce::MathConstants<float>::twoPi;

        juce::Path arc;
        arc.addCentredArc(c.x, c.y, r, r, 0.0f, sa, ea, true);
        g.setColour(Colours::accent);
        g.strokePath(arc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

        const char* labels[] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" };
        g.setColour(Colours::textDark);
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText(labels[juce::jlimit(0, 5, value)], b, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        parameter.beginChangeGesture();
        dragStartY = juce::Desktop::getMousePosition().y;
        dragStartValue = parameter.getIndex();
        dragging = true;
    }

    void mouseDrag(const juce::MouseEvent&) override
    {
        if (!dragging) return;
        int dy = (dragStartY - juce::Desktop::getMousePosition().y) / 22;
        int nv = juce::jlimit(0, 5, dragStartValue + dy);
        if (nv != value)
        {
            value = nv;
            parameter.setValueNotifyingHost(value / 5.0f);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        dragging = false;
        parameter.endChangeGesture();
    }

private:
    juce::AudioParameterChoice& parameter;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int value = 2;
    int dragStartY = 0;
    int dragStartValue = 0;
    bool dragging = false;
};

/* ================================================================
   Rotary Knob — Regen, Mix, Output
   ================================================================ */
class DelayAudioProcessorEditor::RotaryKnob : public juce::Component
{
public:
    RotaryKnob(juce::RangedAudioParameter& param, bool isLarge = false)
        : parameter(param), large(isLarge)
    {
        setSize(isLarge ? 68 : 52, isLarge ? 68 : 52);
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        attachment = std::make_unique<juce::ParameterAttachment>(
            param, [this](float v) { value = v; repaint(); });
        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Dashed track
        juce::Path track; track.addEllipse(b.reduced(2));
        float dl[] = { 3.0f, 3.0f };
        g.setColour(juce::Colour(0x73a0aec0));
        g.strokePath(track, juce::PathStrokeType(1.0f), dl, 2);

        // Knob body
        int ks = large ? 58 : 44;
        auto kb = b.withSizeKeepingCentre((float)ks, (float)ks);

        juce::DropShadow ds(juce::Colours::black.withAlpha(0.12f), 6, {0, 3});
        juce::Path sp; sp.addEllipse(kb); ds.drawForPath(g, sp);

        juce::ColourGradient grad(juce::Colour(0xfff1f6fa),
                                  kb.getX() + kb.getWidth() * 0.35f,
                                  kb.getY() + kb.getHeight() * 0.35f,
                                  juce::Colour(0xffcbdbe7),
                                  kb.getCentreX(), kb.getCentreY(), true);
        g.setGradientFill(grad);
        g.fillEllipse(kb);

        g.setColour(juce::Colour(0xffb8c8d5));
        g.drawEllipse(kb, 1.2f);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawEllipse(kb.reduced(1.5f), 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawEllipse(kb.translated(0, 0.5f), 1.0f);

        // Pointer
        float ang = juce::jmap(value, -135.0f, 135.0f)
                    * juce::MathConstants<float>::pi / 180.0f;
        auto cen = kb.getCentre();
        float rad = kb.getWidth() * 0.35f;
        float px = cen.x + rad * -std::sin(ang);
        float py = cen.y + rad * -std::cos(ang);

        g.setColour(juce::Colour(0xff4a5568));
        float ps = large ? 2.25f : 2.0f;
        g.fillEllipse(px - ps, py - ps, ps * 2.0f, ps * 2.0f);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        parameter.beginChangeGesture();
        dragStartY = juce::Desktop::getMousePosition().y;
        dragStartValue = value;
        dragging = true;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    void mouseDrag(const juce::MouseEvent&) override
    {
        if (!dragging) return;
        int dy = dragStartY - juce::Desktop::getMousePosition().y;
        // 1.6 deg/px  =>  1.6/270 = 0.00593 norm/px
        float nv = juce::jlimit(0.0f, 1.0f, dragStartValue + dy * 0.00593f);
        if (! juce::approximatelyEqual(nv, value))
        {
            value = nv;
            parameter.setValueNotifyingHost(value);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        dragging = false;
        parameter.endChangeGesture();
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

private:
    juce::RangedAudioParameter& parameter;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool large;
    float value = 0.5f;
    int dragStartY = 0;
    float dragStartValue = 0.0f;
    bool dragging = false;
};

/* ================================================================
   Icon Buttons — Brightness, Color, Sparkle
   ================================================================ */
class DelayAudioProcessorEditor::IconButton : public juce::Component
{
public:
    enum Type { Brightness, Color, Sparkle };

    IconButton(juce::AudioParameterBool& param, Type t, const juce::String& lbl)
        : parameter(param), type(t), text(lbl)
    {
        setSize(86, 50); // wider to fit 8px bold labels + padding
        attachment = std::make_unique<juce::ParameterAttachment>(
            param, [this](float v) { state = v > 0.5f; repaint(); });
        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        if (pressed)
            b = b.withSizeKeepingCentre(b.getWidth() * 0.96f, b.getHeight() * 0.96f);

        auto icon = b.removeFromTop(30.0f).withSizeKeepingCentre(30.0f, 30.0f);

        g.setColour((state || hover) ? Colours::accent : juce::Colour(0xff3e4857));

        switch (type)
        {
            case Brightness: drawBrightness(g, icon); break;
            case Color:      drawColor(g, icon);      break;
            case Sparkle:    drawSparkle(g, icon);    break;
        }

        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.setColour(state ? Colours::accent : Colours::textLabel);
        g.drawText(text, b, juce::Justification::centredTop);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        pressed = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        pressed = false;
        parameter.beginChangeGesture();
        parameter.setValueNotifyingHost(state ? 0.0f : 1.0f);
        parameter.endChangeGesture();
        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; if(pressed){pressed=false; repaint();} }

private:
    juce::AudioParameterBool& parameter;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    Type type;
    juce::String text;
    bool state = false;
    bool hover = false;
    bool pressed = false;

    void drawBrightness(juce::Graphics& g, juce::Rectangle<float> b)
    {
        // Circle mask
        juce::Path mask; mask.addEllipse(b.reduced(2));
        g.saveState(); g.reduceClipRegion(mask);

        // Horizontal stripes
        for (int i = 0; i < 9; ++i)
        {
            float y = b.getY() + 8 + i * 3;
            g.drawLine(b.getX(), y, b.getRight(), y, 1.5f);
        }
        g.restoreState();

        // Vertical slit (the mask line in HTML)
        g.setColour(Colours::bottomPanel); // erase with background colour
        float sx = b.getCentreX();
        g.drawLine(sx, b.getY() + 2, sx, b.getBottom() - 2, 1.8f);

        // Outline
        g.setColour((state || hover) ? Colours::accent : juce::Colour(0xff3e4857));
        g.drawEllipse(b.reduced(2), 1.5f);
    }

    void drawColor(juce::Graphics& g, juce::Rectangle<float> b)
    {
        auto cen = b.getCentre();
        float r = 6.0f * (b.getWidth() / 40.0f);
        float o = r * 0.866f;
        juce::Point<float> pts[] = {
            cen, {cen.x, cen.y - r}, {cen.x + o, cen.y - r * 0.5f},
            {cen.x + o, cen.y + r * 0.5f}, {cen.x, cen.y + r},
            {cen.x - o, cen.y + r * 0.5f}, {cen.x - o, cen.y - r * 0.5f}
        };
        for (auto& p : pts)
            g.drawEllipse(p.x - r, p.y - r, r * 2.0f, r * 2.0f, 1.5f);
    }

    void drawSparkle(juce::Graphics& g, juce::Rectangle<float> b)
    {
        auto star = [](juce::Point<float> p, float sz)
        {
            juce::Path path;
            path.startNewSubPath(p.x, p.y - sz);
            path.lineTo(p.x + sz * 0.2f, p.y - sz * 0.2f);
            path.lineTo(p.x + sz, p.y);
            path.lineTo(p.x + sz * 0.2f, p.y + sz * 0.2f);
            path.lineTo(p.x, p.y + sz);
            path.lineTo(p.x - sz * 0.2f, p.y + sz * 0.2f);
            path.lineTo(p.x - sz, p.y);
            path.lineTo(p.x - sz * 0.2f, p.y - sz * 0.2f);
            path.closeSubPath();
            return path;
        };

        float sc = b.getWidth() / 40.0f;
        g.fillPath(star({b.getCentreX(), b.getY() + 10 * sc}, 3.5f * sc * 1.1f));
        g.fillPath(star({b.getX() + 14 * sc, b.getY() + 18 * sc}, 3.5f * sc * 0.85f));
        g.fillPath(star({b.getRight() - 14 * sc, b.getY() + 19 * sc}, 3.5f * sc * 0.95f));
        g.fillPath(star({b.getCentreX(), b.getY() + 27 * sc}, 3.5f * sc * 1.15f));
        g.fillPath(star({b.getX() + 12 * sc, b.getY() + 12 * sc}, 3.5f * sc * 0.6f));
        g.fillPath(star({b.getRight() - 12 * sc, b.getY() + 11 * sc}, 3.5f * sc * 0.6f));
        g.fillPath(star({b.getX() + 12 * sc, b.getY() + 25 * sc}, 3.5f * sc * 0.6f));
        g.fillPath(star({b.getRight() - 12 * sc, b.getY() + 26 * sc}, 3.5f * sc * 0.65f));
    }
};

/* ================================================================
   VU Meter Channel
   ================================================================ */
class DelayAudioProcessorEditor::VUChannel : public juce::Component,
                                             private juce::Timer
{
public:
    VUChannel(std::atomic<float>& lvl) : level(lvl) { setSize(14, 110); startTimerHz(60); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xffe2e8f0));
        g.fillRoundedRectangle(b, 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.1f));
        g.drawRoundedRectangle(b.reduced(0.5f), 2.0f, 1.0f);

        float lh = 2.0f, gap = 1.5f;
        float totalH = 24.0f * (lh + gap) - gap;
        float sy = b.getCentreY() - totalH / 2.0f;
        float lw = 7.0f, lx = b.getCentreX() - lw / 2.0f;

        int lit = juce::jlimit(0, 24, (int)(smoothed * 24.0f));

        for (int i = 0; i < 24; ++i)
        {
            float y = sy + i * (lh + gap);
            juce::Rectangle<float> lr(lx, y, lw, lh);

            if (i < lit)
            {
                juce::Colour c;
                if (i < 16) c = Colours::ledGreen;
                else if (i < 19) c = Colours::ledBlue;
                else if (i < 22) c = Colours::ledOrange;
                else c = Colours::ledRed;

                g.setColour(c);
                g.fillRect(lr);
                juce::DropShadow glow(c.withAlpha(0.8f), 3, {0, 0});
                juce::Path p; p.addRectangle(lr); glow.drawForPath(g, p);
            }
            else
            {
                g.setColour(juce::Colour(0xffcbd5e0));
                g.fillRect(lr);
            }
        }
    }

    void timerCallback() override
    {
        float t = level.load();
        smoothed += (t - smoothed) * (t > smoothed ? 0.35f : 0.12f);
        repaint();
    }

private:
    std::atomic<float>& level;
    float smoothed = 0.0f;
};

/* ================================================================
   Main Editor Assembly
   ================================================================ */
DelayAudioProcessorEditor::DelayAudioProcessorEditor(DelayAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(530, 440);
    setResizable(false, false);

    visualizer   = std::make_unique<WireframeVisualizer>();
    modeToggle   = std::make_unique<ToggleSwitch>(*dynamic_cast<juce::AudioParameterBool*>(p.getAPVTS().getParameter("mode")));
    autoGainBtn  = std::make_unique<AutoGainButton>(*dynamic_cast<juce::AudioParameterBool*>(p.getAPVTS().getParameter("autogain")));
    tempoControl = std::make_unique<TempoControl>(*dynamic_cast<juce::AudioParameterChoice*>(p.getAPVTS().getParameter("tempo")));
    regenKnob    = std::make_unique<RotaryKnob>(*p.getAPVTS().getParameter("regen"), false);
    mixKnob      = std::make_unique<RotaryKnob>(*p.getAPVTS().getParameter("mix"), true);
    outputKnob   = std::make_unique<RotaryKnob>(*p.getAPVTS().getParameter("output"), false);

    brightnessBtn = std::make_unique<IconButton>(*dynamic_cast<juce::AudioParameterBool*>(p.getAPVTS().getParameter("brightness")), IconButton::Brightness, "BRIGHTNESS");
    colorBtn      = std::make_unique<IconButton>(*dynamic_cast<juce::AudioParameterBool*>(p.getAPVTS().getParameter("color")), IconButton::Color, "COLOR");
    sparkleBtn    = std::make_unique<IconButton>(*dynamic_cast<juce::AudioParameterBool*>(p.getAPVTS().getParameter("sparkle")), IconButton::Sparkle, "SPARKLE");

    meterIn  = std::make_unique<VUChannel>(p.getInputLevel());
    meterOut = std::make_unique<VUChannel>(p.getOutputLevel());

    addAndMakeVisible(*visualizer);
    addAndMakeVisible(*modeToggle);
    addAndMakeVisible(*autoGainBtn);
    addAndMakeVisible(*tempoControl);
    addAndMakeVisible(*regenKnob);
    addAndMakeVisible(*mixKnob);
    addAndMakeVisible(*outputKnob);
    addAndMakeVisible(*brightnessBtn);
    addAndMakeVisible(*colorBtn);
    addAndMakeVisible(*sparkleBtn);
    addAndMakeVisible(*meterIn);
    addAndMakeVisible(*meterOut);

    generateNoiseTexture();
    p.getAPVTS().addParameterListener("mode", this);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
    processor.getAPVTS().removeParameterListener("mode", this);
}

void DelayAudioProcessorEditor::generateNoiseTexture()
{
    noiseTexture = juce::Image(juce::Image::ARGB, 200, 200, true);
    juce::Random rng;
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 200; ++x)
            noiseTexture.setPixelAt(x, y, juce::Colour((juce::uint8)rng.nextInt(256),
                                                      (juce::uint8)rng.nextInt(256),
                                                      (juce::uint8)rng.nextInt(256),
                                                      (juce::uint8)5));
}

void DelayAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Outer shadow
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.4f), 40, {0, 20});
    juce::Path sp; sp.addRoundedRectangle(b, 8.0f);
    shadow.drawForPath(g, sp);

    // Main fill
    g.setColour(Colours::mainBg);
    g.fillRoundedRectangle(b, 8.0f);
    g.setColour(Colours::border);
    g.drawRoundedRectangle(b, 8.0f, 1.0f);

    // Inset top highlight
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(1, 1, b.getWidth() - 1, 1, 1.0f);

    // Mid bar
    juce::Rectangle<float> mid(0, 160, b.getWidth(), 38);
    g.setColour(Colours::midBar);
    g.fillRect(mid);
    g.setColour(juce::Colour(0xffe1e4e8));
    g.drawLine(0, 198, b.getWidth(), 198, 1.0f);

    // Bottom panel
    g.setColour(Colours::bottomPanel);
    g.fillRect(0, 198, b.getWidth(), b.getHeight() - 198);

    // Right divider
    float rx = b.getWidth() * 0.78f;
    g.setColour(juce::Colour(0xffe1e4e8));
    g.drawLine(rx, 198, rx, b.getHeight(), 1.0f);

    // Mode labels — exact mid-bar padding 0 24px
    bool creative = dynamic_cast<juce::AudioParameterBool*>(processor.getAPVTS().getParameter("mode"))->get();
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(creative ? Colours::textLabel : Colours::textDark);
    g.drawText("STUDIO", 24, 168, 50, 20, juce::Justification::left);
    g.setColour(creative ? Colours::textDark : Colours::textLabel);
    g.drawText("CREATIVE", 116, 168, 60, 20, juce::Justification::left);

    // Knob labels — positioned under their exact centres
    g.setColour(Colours::textLabel);
    auto cx = [&](auto& c) { return c->getBounds().getCentreX(); };
    g.drawText("TEMPO",  cx(tempoControl) - 25, tempoControl->getBottom() + 4, 50, 12, juce::Justification::centred);
    g.drawText("REGEN",  cx(regenKnob) - 25, regenKnob->getBottom() + 4, 50, 12, juce::Justification::centred);
    g.drawText("MIX",    cx(mixKnob) - 25, mixKnob->getBottom() + 4, 50, 12, juce::Justification::centred);
    g.drawText("OUTPUT", cx(outputKnob) - 30, outputKnob->getBottom() + 4, 60, 12, juce::Justification::centred);

    // Meter labels
    g.drawText("IN",  meterIn->getBounds().getCentreX() - 10, meterIn->getBottom() + 2, 20, 10, juce::Justification::centred);
    g.drawText("OUT", meterOut->getBounds().getCentreX() - 15, meterOut->getBottom() + 2, 30, 10, juce::Justification::centred);

    // Noise overlay
    g.setOpacity(0.02f);
    g.drawImage(noiseTexture, b);
    g.setOpacity(1.0f);
}

void DelayAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    // Logo with scaled arrow
    g.setColour(Colours::accent.withAlpha(0.9f));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("DELAY", 20, 16, 60, 20, juce::Justification::left);

    // Arrow scaled Y 0.7
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(1.0f, 0.7f, 78.0f, 16.0f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("▼", 78, 16, 20, 20, juce::Justification::left);
    g.restoreState();
}

void DelayAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();

    visualizer->setBounds(0, 0, b.getWidth(), 160);

    // Mid bar — exact padding 0 24px
    modeToggle->setBounds(74, 170, 32, 17);
    autoGainBtn->setBounds(b.getWidth() - 24 - 50, 168, 50, 20);

    // Layout maths
    int leftPanelW = (int)(b.getWidth() * 0.78f);
    int leftPad = 24;
    int contentW = leftPanelW - leftPad * 2;

    // Knobs: space-between
    int knobY = 222; // 198 + 24 padding
    int tempoW = 66, regenW = 52, mixW = 68;
    int knobGap = (contentW - tempoW - regenW - mixW) / 2;
    tempoControl->setBounds(leftPad, knobY, tempoW, 66);
    regenKnob->setBounds(leftPad + tempoW + knobGap, knobY + 7, regenW, regenW); // +7 to visually centre with 66px tempo
    mixKnob->setBounds(leftPad + tempoW + knobGap + regenW + knobGap, knobY, mixW, mixW);

    // Buttons: space-between
    int btnY = 370; // 440 - 20 bottom pad - 50 height
    int btnW = 86;
    int btnGap = (contentW - btnW * 3) / 2;
    brightnessBtn->setBounds(leftPad, btnY, btnW, 50);
    colorBtn->setBounds(leftPad + btnW + btnGap, btnY, btnW, 50);
    sparkleBtn->setBounds(leftPad + btnW * 2 + btnGap * 2, btnY, btnW, 50);

    // Right panel: padding 20px 0 16px 0
    int rightX = leftPanelW;
    int rightW = b.getWidth() - rightX;
    int meterY = 218; // 198 + 20
    int meterX = rightX + (rightW - 35) / 2;
    meterIn->setBounds(meterX, meterY, 14, 110);
    meterOut->setBounds(meterX + 21, meterY, 14, 110);

    outputKnob->setBounds(rightX + (rightW - 52) / 2, 372, 52, 52); // 440 - 16 - 52 = 372
}

void DelayAudioProcessorEditor::parameterChanged(const juce::String& id, float)
{
    if (id == "mode")
        repaint();
}