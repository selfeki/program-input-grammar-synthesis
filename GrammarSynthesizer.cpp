

#include <string>
#include <list>

class Node {
    virtual void accept(GeneralizeVisitor& visitor) const = 0;
 };

class RepNode : public Node {
    public:
    RepNode(std::string s) 
        : terminal{s} 
        {}

    virtual void accept(GeneralizeVisitor& visitor) const { return visitor.visit(*this); }

    std::string terminal;
};

class AltNode : public Node {
    AltNode(std::string s) 
        : terminal{s} 
        {}
    std::string terminal;
};

class StarNode : public Node {
    std::list<Node> children;
};

class PlusNode : public Node {

    virtual void accept(GeneralizeVisitor& visitor) const { return visitor.visit(*this); }
    Node left;
    Node right;
};

class GeneralizeVisitor {
    public:

    void 
    visit(const RepNode& repNode) { 
        std::string alpha = repNode.terminal;
        std::string sub1, sub2, sub3;

        // primary priority for shorter sub1
        for (int i = 0; i < alpha.size(); i++ ) {
            // sedondary priority for longer sub2
            for (int j = alpha.size; j > i; j--) {
                //TODO implement checks 
            }
        }
    }
    virtual void visit(const AltNode& altNode) { }
};


class GrammarSynthesizer {
    GrammarSynthesizer(std::string s) {
        RepNode* firstCandidate = new RepNode(s);
        grammar = *firstCandidate;
    }

    // TODO: generalize grammar
    
    private:
    Node grammar;
};


int main() {
    return 0;
}