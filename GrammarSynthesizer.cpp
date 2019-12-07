
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
#include <glibmm/ustring.h>


namespace grammarSynthesizer {

class Node;
class RepNode;
class AltNode;
class TerminalNode;
class NonGeneralizableNode;


typedef std::vector<Node*> Grammar;


class Oracle {
public:
	virtual bool query(std::string check) = 0;
};

class NodeVisitor {
public:
  virtual Grammar visit(RepNode* repNode) { return {}; }
  virtual Grammar visit(AltNode* altNode) {	return {}; }
  virtual Grammar visit(TerminalNode* terminalNode) {	return {}; }
	virtual Grammar visit(NonGeneralizableNode* starNode) {	return {}; }
};


class Node {
public:
	virtual Grammar accept(NodeVisitor &visitor) = 0;
};


class RepNode : public Node {
public:
	RepNode(std::string s)
		: terminal{s}
		{}
	std::string getTerminal() { return terminal; }
	virtual Grammar accept(NodeVisitor &visitor) { return visitor.visit(this); }

private:
	Node *parent;
	std::string terminal;
};


class AltNode : public Node {
public:
	AltNode(std::string s)
		: terminal{s}
		{}
	std::string getTerminal() { return terminal; }
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

private:
	Node *parent;
	std::string terminal;
};


class NonGeneralizableNode : public Node {
public:
	NonGeneralizableNode(){}
	void addChild(Node* node) { grammar.push_back(node); }
	Grammar getGrammar() { return grammar; }
	void setGrammar(Grammar newGrammar) { grammar = newGrammar; }
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

private:
	Node* parent;
	Grammar grammar;
};


class TerminalNode : public Node {
public:
	TerminalNode(std::string s)
		: terminal{s}
		{}
	std::string getTerminal() { return terminal; }
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }
 
private:
	Node* parent;
	std::string terminal;
};


class StarNode : public NonGeneralizableNode {};


class PlusNode : public NonGeneralizableNode {};

struct Context {
	std::string left;
	std::string right;
};


class GetContextVisitor: public NodeVisitor {
public:
	GetContextVisitor(Node* r, Node* t)
		: root(r),
			target(t)
		{ }
	
	Grammar
	visit(StarNode* starNode) {
		for (Node* node : starNode->getGrammar()) {
			node->accept(*this);
		}
		return {};
	}

	Grammar
	visit(PlusNode* plusNode) {
		for (Node* node : plusNode->getGrammar()) {
			node->accept(*this);
		}
		return {};
	}

	Grammar
	visit(TerminalNode* terminalNode) {
		currentCtxt->append(terminalNode->getTerminal());
		return {};
	}

	Grammar
	visit(RepNode* repNode) {
		if (repNode == target) {
			currentCtxt = &ctxt.right;
			return {};
		}
		currentCtxt->append(repNode->getTerminal());
		return {};
	}

	Grammar
	visit(AltNode* altNode) {
		if (altNode == target) {
			currentCtxt = &ctxt.right;
			return {};
		}
		currentCtxt->append(altNode->getTerminal());
		return {};
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
	GeneralizeVisitor(Node* r, Oracle& orcl)
	: root(r),
		oracle(orcl)
	{}

	std::vector<std::string>
	generateContexts(std::string sub1, std::string sub2, std::string sub3) {
		return {sub1.append(sub3), sub1.append(sub2).append(sub2).append(sub3)};
	}

	void
	replaceGrammarAt(Grammar& grammar, Grammar& newGrammar, Node* toReplace) { // TODO: bug hotspot
		auto it 		= std::find (grammar.begin(), grammar.end(), toReplace);
		auto newLoc = grammar.erase(it);
		grammar.insert(newLoc, newGrammar.begin(), newGrammar.end());
	}

	Grammar
	visit(TerminalNode* terminalNode) { return {}; }

	Grammar
	visit(NonGeneralizableNode* nonGenNode) {
		Grammar grammar = nonGenNode->getGrammar();
		for (auto it = grammar.rbegin(); it != grammar.rend(); ++it) {
			Node* node = *it;
			Grammar resGrammar = node->accept(*this);
			if (isGeneralized) {
				replaceGrammarAt(grammar, resGrammar, node);
				break;
			}
		}
		return {};
	}

	std::vector<std::string>
	getRepResiduals(std::string sub1, std::string sub2, std::string sub3) {
		return {sub1.append(sub3), sub1.append(sub2).append(sub2).append(sub3)};
	}

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
				std::vector<std::string> residuals = getRepResiduals(sub1, sub2, sub3);

				for (std::string res : residuals) {
					std::string check = context.left.append(res).append(context.right);
					if (oracle.query(check)) {
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

	Grammar
	visit(AltNode* altNode) {
		if (isGeneralized) { return {}; }
		GetContextVisitor getContextVisitor(root, altNode);
		root->accept(getContextVisitor);
		Context context = getContextVisitor.getContext();

		std::string alpha = altNode->getTerminal();
		std::string sub1, sub2;
		int length = alpha.length();
		Grammar candidate;
		// priority for shorter sub1
		for (int i = 1; i < length; i++) {
			sub1 = alpha.substr(0, i);
			sub2 = alpha.substr(i, length-i);
			std::vector<std::string> residuals = {sub1, sub2};

			for (std::string res : residuals) {
				std::string check = context.left.append(res).append(context.right);
				if (oracle.query(check)) {

					RepNode* rep	 = new RepNode(sub1);
					AltNode* alt	 = new AltNode(sub2);
					PlusNode* plus = new PlusNode();
					plus->addChild(rep);
					plus->addChild(alt);

					candidate.push_back(plus);
					isGeneralized = true;
					return candidate;
				}
			}
		}
		// last resort generalization
		candidate.push_back(new RepNode(alpha));
		isGeneralized = true;
		return candidate;
	}

	bool checkGeneralized() { return isGeneralized; }

private:
	Node* root;
	bool isGeneralized = false;
	Oracle& oracle;
};


class PrintVisitor : public NodeVisitor {
public:
  Grammar
	visit(RepNode* repNode) { 
		std::cout << "[ " << repNode->getTerminal() << " ]rep";
		return {};
	}

	Grammar
	visit(AltNode* altNode) { 
		std::cout << "[ " << altNode->getTerminal() << " ]alt";
		return {};
	}

	Grammar
	visit(PlusNode* plusNode) {
		Grammar grammar = plusNode->getGrammar();
		Node* left  = grammar[0];
		Node* right = grammar[1];
		std::cout << "( ";
		left->accept(*this);
		std::cout << " + ";
		right->accept(*this);
		std::cout << " )";
		return {};
	}

	Grammar
	visit(StarNode* starNode) {
		std::cout << "( ";
		for (Node* node : starNode->getGrammar()) {
			node->accept(*this);
		}
		std::cout << " )*";
		return {};
	}

	Grammar
	visit(TerminalNode* terminalNode) { 
		std::cout << terminalNode->getTerminal();
		return {};
	}
};


class GrammarSynthesizer {
public:
	GrammarSynthesizer(std::string seed, Oracle& orcl)
		: grammar({new RepNode(seed)}) ,
			oracle(orcl)
		{}

	Grammar
	synthesize() {
		bool isGeneralized = true;

		while (isGeneralized) {
			isGeneralized = false;

			for (int i = grammar.size() - 1; i >= 0; i--) {
				Node* root = grammar[i];
				GeneralizeVisitor generalizeVisitor(root, oracle);
				Grammar newGrammar = root->accept(generalizeVisitor);

				if (generalizeVisitor.checkGeneralized()) {
					isGeneralized = true;
					if (newGrammar.size() > 0) {
						grammar.insert(grammar.begin()+i, newGrammar.begin(), newGrammar.end());
					}
					break;
				}
			}
		}
		return grammar;
	}

private:
	std::vector<Node *> grammar;
	Oracle& oracle;
};


}

using namespace grammarSynthesizer;


class XMLOracle : public Oracle{
public:
	virtual bool query(std::string check) {
		try{
			parser.parse_memory(check);
		}
		catch(const std::exception& e) {
			std::cout << "check failed:	"<< check << std::endl;
			return false;
		}
		return true;
	}

	private:
	xmlpp::DomParser parser;
};

int main() {
	std::string seed = "<to>Tove</to>";
	XMLOracle oracle;
	GrammarSynthesizer gs(seed, oracle);
	Grammar grammar = gs.synthesize();

	PrintVisitor ps;
	for (Node* node : grammar) {
		node->accept(ps);
	}
	return 0;
}