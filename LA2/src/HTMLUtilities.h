#ifndef HTML_UTILITIES_H
#define HTML_UTILITIES_H

#include <Arduino.h>
#include <vector>

// A simple HTML element class
class HTMLElement {
    private:
        String tag;
        String content;
        std::vector<String> attributes;
    
    public:
        HTMLElement(String t) : tag(t) {}
        
        // Add content inside the tag
        HTMLElement& setContent(String c) {
            content = c;
            return *this;
        }
        
        // Add an attribute (e.g., class="example")
        HTMLElement& addAttribute(String attr) {
            attributes.push_back(attr);
            return *this;
        }
        
        // Convert to string
        String toString() {
            String result = "<" + tag;
            
            // Add attributes
            for (String attr : attributes) {
                result += " " + attr;
            }
            
            // Self-closing tag or regular tag with content
            if (content.length() == 0 && (tag == "input" || tag == "img" || tag == "br" || tag == "hr")) {
                result += " />";
            } else {
                result += ">" + content + "</" + tag + ">";
            }
            
            return result;
        }
};

// HTML document class
class HTMLDocument {
    private:
        String title;
        String cssStyles;
        std::vector<String> bodyContent;
        String charset = "utf-8";
        String viewport = "width=device-width, initial-scale=1.0";
    
    public:
        HTMLDocument(String t) : title(t) {}
        
        // Set meta charset
        HTMLDocument& setCharset(String c) {
            charset = c;
            return *this;
        }
        
        // Set viewport
        HTMLDocument& setViewport(String v) {
            viewport = v;
            return *this;
        }
        
        // Add CSS styles
        HTMLDocument& addStyles(String css) {
            cssStyles += css;
            return *this;
        }
        
        // Add content to body
        HTMLDocument& addToBody(String content) {
            bodyContent.push_back(content);
            return *this;
        }
        
        // Generate the complete HTML document
        String toString() {
            String html = "<!DOCTYPE html><html><head>";
            html += "<title>" + title + "</title>";
            html += "<meta charset=\"" + charset + "\">";
            html += "<meta name=\"viewport\" content=\"" + viewport + "\">";
            
            if (cssStyles.length() > 0) {
                html += "<style>" + cssStyles + "</style>";
            }
            
            html += "</head><body>";
            
            for (String content : bodyContent) {
                html += content;
            }
            
            html += "</body></html>";
            return html;
        }
};

#endif
