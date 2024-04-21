#include <iostream>
#include <X11/Xlib.h>
#include <vector>
#include <unordered_map>

enum ButtonType {
  PRIMARY, SECONDARY, TERTIARY
};

class Component {
  public:
    Component(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height) {}
    virtual ~Component() {}

    virtual void Draw(Display* display, Window window) = 0;
    virtual void HandleEvent(const std::string& event, int x, int y) = 0;

  protected:
    int m_x;
    int m_y;
    int m_width;
    int m_height;
};

typedef struct Color {
  XColor textColor;
  XColor bgColor;
};

class Button : public Component {
  public:
    Button(int x, int y, int width, int height, const std::string& text);
    ~Button() {}

    void Draw(Display* display, Window window) override;
    void HandleEvent(const std::string& event, int x, int y) override;

    Color SetType(Display* display, const ButtonType& type);

  private:
    std::string m_text;
    std::string m_type; // For color
    Color color;
    GC* m_gc;
    Display* m_display;

    int m_marginX = 12;
    int m_marginY;

    int countTextPixelSize();

    bool m_gcInitialized = false;
};

class AppWindow {
  public:
    AppWindow();
    ~AppWindow();

    void addComponent(Component* component);
    void DrawComponents(Display* display, Window window);
    void HandleComponentEvent(const std::string& event, int x, int y);

  private:
    std::vector<Component*> m_components;
};

Button::Button(int x, int y, int width, int height, const std::string& text) : Component(x, y, width, height), m_text(text) {}

void Button::Draw(Display* display, Window window) {
  if (!m_gcInitialized) {
    m_gc = new GC();
    XGCValues values;
    values.foreground = BlackPixel(display, DefaultScreen(display));
    values.line_width = 1;
    values.line_style = LineSolid;
    *m_gc = XCreateGC(display, window, GCForeground | GCLineWidth | GCLineStyle, &values);

    color = SetType(display, PRIMARY);
    m_gcInitialized = true;
    m_display = display;
  }

  // Draw Border
  XSetForeground(display, *m_gc, color.textColor.pixel);
  XDrawRectangle(display, window, *m_gc, m_x - 1, m_y - 1, countTextPixelSize() + m_marginX + 1, m_height + m_marginY + 1);
  
  // Draw Background
  XSetForeground(display, *m_gc, color.bgColor.pixel);
  XFillRectangle(display, window, *m_gc, m_x, m_y, countTextPixelSize() + m_marginX, m_height + m_marginY);
  
  // Draw Text
  XSetForeground(display, *m_gc, color.textColor.pixel);
  XDrawString(display, window, *m_gc, m_x + m_marginX / 2, m_y + m_height / 2 + 5, m_text.c_str(), m_text.length());

  XFlush(display);
}

void Button::HandleEvent(const std::string& event, int x, int y) {
  if (event == "ButtonPress") {
    if (x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + m_height) {
      std::cout << "Button pressed" << std::endl;
    }
  }

  if (event == "ButtonRelease") {
    if (x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + m_height) {
      std::cout << "Button released" << std::endl;
    }
  }

  if (event == "MotionNotify") {
    if (x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + m_height) {
      // TODO: Change button color when mouse hover
      // if (m_display != nullptr) {
        this->SetType(m_display, SECONDARY);
        std::cout << "Mouse hover" << std::endl;
      // }
    } else {
      // if (m_display != nullptr) {
        this->SetType(m_display, PRIMARY);
      // }
    }
  }
}

Color Button::SetType(Display* display, const ButtonType& type) {
  Colormap colormap = DefaultColormap(display, DefaultScreen(display));
  XColor m_textColor, m_bgColor;

  switch (type) {
    case PRIMARY:
      XAllocNamedColor(display, colormap, "black", &m_textColor, &m_textColor);
      XAllocNamedColor(display, colormap, "white", &m_bgColor, &m_bgColor);
      break;
    case SECONDARY:
      XAllocNamedColor(display, colormap, "blue", &m_textColor, &m_textColor);
      XAllocNamedColor(display, colormap, "yellow", &m_bgColor, &m_bgColor);
      break;
    case TERTIARY:
      XAllocNamedColor(display, colormap, "black", &m_textColor, &m_textColor);
      XAllocNamedColor(display, colormap, "white", &m_bgColor, &m_bgColor);
      break;
  }
  
  color.textColor = m_textColor;
  color.bgColor = m_bgColor;

  return color;
}

int Button::countTextPixelSize() {
  XFontStruct* fontInfo = XLoadQueryFont(m_display, "fixed");
  if (!fontInfo) {
    std::cerr << "Failed to load font" << std::endl;
    return 0;
  }

  int text_width = XTextWidth(fontInfo, m_text.c_str(), m_text.length());
  XFreeFontInfo(NULL, fontInfo, 0);
  return text_width;
}


AppWindow::AppWindow() {}

AppWindow::~AppWindow() {
  for (auto component : m_components) {
    delete component;
  }
}

void AppWindow::addComponent(Component* component) {
  m_components.push_back(component);
}

void AppWindow::DrawComponents(Display* display, Window window) {
  for (auto component : m_components) {
    component->Draw(display, window);
  }
}

void AppWindow::HandleComponentEvent(const std::string& event, int x, int y) {
  for (auto component : m_components) {
    component->HandleEvent(event, x, y);
  }
}

int main(int argc, char** argv) {
  Display* display = XOpenDisplay(NULL);
  if (display == NULL) {
    std::cerr << "Cannot open display" << std::endl;
    return 1;
  }

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);
  XWindowAttributes windowAttributes;
  XGetWindowAttributes(display, root, &windowAttributes);

  /*
   * Create main window where we can put our components.
   * There are two types of windows: main window and child window that
   * act as a component of the main window.
   */
  Window window = XCreateSimpleWindow(display, root, 0, 0, 640, 480, 1,
                                      BlackPixel(display, screen),
                                      WhitePixel(display, screen));

  /* 
   * Detect mouse motion, mouse button, etc. so that we can create 
   * component that can interact with user like hover, click, etc.
   */
  XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XMapWindow(display, window);

  /*
   * Attach X window to AppWindow class.
   */
  AppWindow appWindow;
  
  std::vector<std::string> buttonNames = {"File", "Edit", "Help", "About"};

  for (int i = 0; i < buttonNames.size(); i++) {
    Button* button = new Button(i * 37, 0, 0, 24, buttonNames[i]);
    appWindow.addComponent(button);
  }

  // Event loop
  XEvent event;
  while (1) {
    XNextEvent(display, &event);
    if (event.type == Expose) {
      // Redraw window
      appWindow.DrawComponents(display, window);
    }

    if (event.type == ButtonPress) {
      // Handle button press event
      appWindow.HandleComponentEvent("ButtonPress", event.xbutton.x, event.xbutton.y);
    }

    if (event.type == ButtonRelease) {
      // Handle button release event
      appWindow.HandleComponentEvent("ButtonRelease", event.xbutton.x, event.xbutton.y);
    }

    if (event.type == MotionNotify) {
      // Handle mouse motion event
      appWindow.HandleComponentEvent("MotionNotify", event.xmotion.x, event.xmotion.y);
    }
  }


  XCloseDisplay(display);

  return 0;
}