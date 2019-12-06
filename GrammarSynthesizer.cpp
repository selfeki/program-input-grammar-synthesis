

#include <string>
#include <list>
#include <vector>
#include <functional>

class Node {
public:
	virtual Grammar accept(NodeVisitor &visitor) = 0;
};


class RepNode : public Node {
public:
	RepNode(std::string s)
		: terminal{s}
		{}

	virtual Grammar accept(NodeVisitor &visitor) { return visitor.visit(this); }

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

	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::string getTerminal() { return terminal; }

private:
	Node *parent;
	std::string terminal;
};


class StarNode : public Node {
public:
		StarNode(){}

	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::list<Node*> getChildren() { return children; }

	void addChild(Node* node) { children.push_back(node); }

private:
	Node* parent;
	std::list<Node*> children;
};


class PlusNode : public Node {
public:
	PlusNode(){}

	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::list<Node*> getChildren() { return children; }

	void addChild(Node* node) { children.push_back(node); }
	
private:
	Node* parent;
	std::list<Node*> children;
};


class TerminalNode : public Node {
public:
	TerminalNode(std::string s)
		: terminal{s}
		{}

	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

	std::string getTerminal() { return terminal; }
	
private:
	Node* parent;
	std::string terminal;
};

struct Context {
	std::string left;
	std::string right;
};

typedef std::vector<Node*> Grammar;

class NodeVisitor {
public:
  virtual Grammar visit(RepNode* repNode) { }
  virtual Grammar visit(AltNode* altNode) { }
  virtual Grammar visit(StarNode* starNode) { }
  virtual Grammar visit(PlusNode* plusNode) { }
  virtual Grammar visit(TerminalNode* terminalNode) { }
};


class GetContextVisitor: public NodeVisitor {

public:
	GetContextVisitor(Node* r, Node* t)
		: root(r),
			target(t)
		{ }
	
	Grammar
	visit(StarNode* starNode) {
		for (Node* child : starNode->getChildren()) {
			child->accept(*this);
		}
	}

	Grammar
	visit(PlusNode* plusNode) {
		for (Node* child : plusNode->getChildren()) {
			child->accept(*this);
		}
	}

	Grammar
	visit(TerminalNode* terminalNode) {
		currentCtxt->append(terminalNode->getTerminal());
	}

	Grammar
	visit(RepNode* repNode) {
		if (repNode == target) {
			currentCtxt = &ctxt.right;
			return;
		}
		currentCtxt->append(repNode->getTerminal());
	}

	Grammar
	visit(AltNode* altNode) {
		if (altNode == target) {
			currentCtxt = &ctxt.right;
			return;
		}
		currentCtxt->append(altNode->getTerminal());
	}

	Context getContext() { return ctxt; }

private:
	Node* 	root;
	Node* 	target;
	Context ctxt;
	std::string* currentCtxt = &ctxt.left;
};


class GeneralizeVisitor : public NodeVisitor {
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


	Grammar
	visit(RepNode* repNode) {
		if (isGeneralized) { return {}; }
		GetContextVisitor getContextVisitor(root, repNode);
		root->accept(getContextVisitor);
		Context context = getContextVisitor.getContext();

		std::string alpha = repNode->getTerminal();
		std::string sub1, sub2, sub3;
		int length = alpha.length();
		Grammar candidate;
		// primary priority for shorter sub1
		for (int i = 0; i < length; i++) {
			// sedondary priority for longer sub2
			for (int j = length; j > i; j--) { // todo: debug explicitly
				sub1 = alpha.substr(0, i);
				sub2 = alpha.substr(i, j-i);
				sub3 = alpha.substr(j, length - j);
				std::vector<std::string> residuals = generateResiduals(sub1, sub2, sub3);

				for (std::string res : residuals) {
					std::string check = context.left.append(res).append(context.right);
					if (oracle(check)) {
						TerminalNode* decmp1 = new TerminalNode(sub1);
						StarNode* 		decmp2 = new StarNode();
						RepNode* 			decmp3 = new RepNode(sub3);
						decmp2->addChild(new AltNode(sub2));

						candidate.push_back(decmp1);
						candidate.push_back(decmp2);
						candidate.push_back(decmp3);
						isGeneralized = true;
						return candidate;
					}
				}

			}
		}
		// last resort generalization
		candidate.push_back(new TerminalNode(alpha));
		isGeneralized = true;
		return candidate;
	}
	virtual void visit(AltNode &altNode) {}

	Node *root;
	bool isGeneralized = false;
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