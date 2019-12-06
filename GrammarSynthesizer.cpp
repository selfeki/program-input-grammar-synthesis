

#include <string>
#include <list>
#include <vector>
#include <functional>

class Node {
public:
	virtual void accept(NodeVisitor &visitor) = 0;
};


class RepNode : public Node {
public:
	RepNode(std::string s)
		: terminal{s}
		{}

	virtual void accept(NodeVisitor &visitor) { return visitor.visit(this); }

	std::string getTerminal() { return terminal; }

private:
	Node *parent;
	std::string terminal;
};


class AltNode : public Node {
public:
	AltNode(std::string s)
		: terminal{s}
		{}

	virtual void accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::string getTerminal() { return terminal; }

private:
	Node *parent;
	std::string terminal;
};


class StarNode : public Node {
public:
	virtual void accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::list<Node*> getChildren() { return children; }

private:
	Node* parent;
	std::list<Node*> children;
};


class PlusNode : public Node {
public:

	virtual void accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::list<Node*> getChildren() { return children; }
	
private:
	Node* parent;
	std::list<Node*> children;
};


class TerminalNode : public Node {
public:
	virtual void accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::string getTerminal() { return terminal; }
	
private:
	Node* parent;
	std::string terminal;
};

struct Context {
	std::string left;
	std::string right;
};


class NodeVisitor {
public:
  virtual void visit(RepNode* repNode) { }
  virtual void visit(AltNode* altNode) { }
  virtual void visit(StarNode* starNode) { }
  virtual void visit(PlusNode* plusNode) { }
  virtual void visit(TerminalNode* terminalNode) { }
};


class getContextVisitor: public NodeVisitor {
	
	Node* root;
	Node* target;

	void
	visit(StarNode* starNode) {
		for (Node* child : starNode->getChildren()) {
			child->accept(*this);
		}
	}

	void
	visit(PlusNode* plusNode) {
		for (Node* child : plusNode->getChildren()) {
			child->accept(*this);
		}
	}

	void
	visit(TerminalNode* terminalNode) {
		currentCtxt->append(terminalNode->getTerminal());
	}

	void
	visit(RepNode* repNode) {
		if (repNode == target) {
			currentCtxt = &ctxt.right;
			return;
		}
		currentCtxt->append(repNode->getTerminal());
	}

	void
	visit(AltNode* altNode) {
		if (altNode == target) {
			currentCtxt = &ctxt.right;
			return;
		}
		currentCtxt->append(altNode->getTerminal());
	}

	std::string* currentCtxt = &ctxt.left;
	Context ctxt;
};

class GeneralizeVisitor {
public:
	GeneralizeVisitor(Node *r, bool (*predFunc)(std::string))
	: root(r),
		oracle(predFunc)
	{}

	std::vector<std::string>
	generateResiduals(std::string sub1, std::string sub2, std::string sub3) {
		return {sub1.append(sub3), sub1.append(sub2).append(sub2).append(sub3)};
	}

	std::vector<std::string>
	generateContexts(std::string sub1, std::string sub2, std::string sub3) {
		return {sub1.append(sub3), sub1.append(sub2).append(sub2).append(sub3)};
	}

	// std::vector<std::string>
	// generateChecks() {
	// 	return
	// }


	void
	visit(const RepNode *repNode) {

		CreateContextVisitor getContextVisitor(root, repNode);
		root->accept(getContextVisitor);

		Context context = getContextVisitor.context;

		std::string alpha = repNode->terminal;
		std::string sub1, sub2, sub3;

		// primary priority for shorter sub1
		for (int i = 0; i < alpha.size(); i++) {
			// sedondary priority for longer sub2
			for (int j = alpha.size; j > i; j--) {
				//TODO implement checks
			}
		}
		// TODO: need 1 last generalization
		// in case none of the above work
	}
	virtual void visit(const AltNode &altNode) {}

	Node *root;
	std::function<bool (std::string)> oracle;
};

class GrammarSynthesizer {
	GrammarSynthesizer(std::string seed)
		: grammar({new RepNode(seed)}) 
		{}

	std::vector<Node *>
	synthesizeGrammar() {
		bool isGeneralizable = true; // todo: fix this later

		while (isGeneralizable) {

			for (int i = grammar.size() - 1; i >= 0; i--) {
				Node *root = grammar[i];
				GeneralizeVisitor generalizeVisitor(root);
				root->accept(generalizeVisitor);

				if (generalizeVisitor.foundGeneralizable()) {
					isGeneralizable = true;

					// below is unnessesary
					// std::vector<Node*> generalized =  generalizeVisitor.generalizedGrammar;
					// grammar.insert(grammar.begin()+i, generalized.begin(), generalized.end());
					break;
				}
			}
		}
	}

private:
	std::vector<Node *> grammar;
};

int main() {
	return 0;
}