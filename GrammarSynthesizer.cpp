
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <unordered_set>

#include <libxml++/libxml++.h>

namespace grammarSynthesizer {

class Node;
class RepNode;
class AltNode;
class TerminalNode;
class NonGeneralizableNode;
class StarNode;
class PlusNode;


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
	virtual Grammar visit(NonGeneralizableNode* nonGenNode) {	return {}; }
	virtual Grammar visit(StarNode* starNode) {	return {}; }
	virtual Grammar visit(PlusNode* plusNode) {	return {}; }
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
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }

	Grammar 
	getGrammar() { 
		return grammar; 
	}
	
	void 
	setGrammar(Grammar newGrammar) { 
		grammar = newGrammar; 
	}

	void 
	addChild(Node* node) { 
		grammar.push_back(node); 
	}

	void
	replaceGrammarAt(Grammar& newGrammar, Node* toReplace) {
		auto it 		= std::find (grammar.begin(), grammar.end(), toReplace);
		auto newLoc = grammar.erase(it);
		grammar.insert(newLoc, newGrammar.begin(), newGrammar.end());
	}

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


class StarNode : public NonGeneralizableNode {
public:
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }
};


class PlusNode : public NonGeneralizableNode {
public:
	virtual Grammar accept(NodeVisitor& visitor) { return visitor.visit(this); }
};

struct Context {
	std::string left;
	std::string right;
};


class PrintVisitor : public NodeVisitor {
public:
  Grammar
	visit(RepNode* repNode) {
		std::string terminal = repNode->getTerminal();
		// if (terminal.length() > 0) {
			std::cout << "[ " << terminal << " ]rep";
		// }
		return {};
	}

	Grammar
	visit(AltNode* altNode) { 
		std::string terminal = altNode->getTerminal();
		// if (terminal.length() > 0) {
			std::cout << "[ " << terminal << " ]alt";
		// }		
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

class ToStringVisitor : public NodeVisitor {
public:
  Grammar
	visit(RepNode* repNode) {
		std::string terminal = repNode->getTerminal();
		result.append(terminal);
		return {};
	}

	Grammar
	visit(AltNode* altNode) { 
		std::string terminal = altNode->getTerminal();
		result.append(terminal);		
		return {};
	}

	Grammar
	visit(PlusNode* plusNode) {
		Grammar grammar = plusNode->getGrammar();
		Node* left  = grammar[0];
		Node* right = grammar[1];
		result.append("( ");
		left->accept(*this);
		result.append(" + ");
		right->accept(*this);
		result.append(" )");
		return {};
	}

	Grammar
	visit(StarNode* starNode) {
		result.append("( ");
		for (Node* node : starNode->getGrammar()) {
			node->accept(*this);
		}
		result.append(" )*");
		return {};
	}

	Grammar
	visit(TerminalNode* terminalNode) { 
		result.append(terminalNode->getTerminal());
		return {};
	}

	std::string
	getString() { return result; }

private:
	std::string result;
};

class GetContextVisitor: public NodeVisitor {
public:
	GetContextVisitor(Grammar& g, Node* t)
		: grammar(g),
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

	Context 
	getContext() { 
		for (Node* node : grammar) {
			node->accept(*this);
		}
		return ctxt;
	 }

private:
	Grammar&	grammar;
	Node* 		target;
	Context 	ctxt;
	std::string* currentCtxt = &ctxt.left;
};


class GeneralizeVisitor : public NodeVisitor {
public:
	GeneralizeVisitor(Oracle& orcl)
	: oracle(orcl)
	{ }

	std::vector<std::string>
	generateContexts(std::string sub1, std::string sub2, std::string sub3) {
		return {sub1.append(sub3), sub1.append(sub2).append(sub2).append(sub3)};
	}

	Grammar
	visit(TerminalNode* terminalNode) { return {}; }

	Grammar
	visit(StarNode* starNode) {
		std::cout << "grammar replacement - before: ";
		PrintVisitor pv;
		starNode->accept(pv);
		std::cout << std::endl;
		Grammar grammar = starNode->getGrammar();
		for (auto it = grammar.rbegin(); it != grammar.rend(); ++it) {
			Node* node = *it;
			Grammar newGrammar = node->accept(*this);
			if (isGeneralized) {
				starNode->replaceGrammarAt(newGrammar, node);
				break;
			}
		}
		return {};
	}

	Grammar
	visit(PlusNode* plusNode) {
		Grammar grammar = plusNode->getGrammar();
		for (auto it = grammar.rbegin(); it != grammar.rend(); ++it) {
			Node* node = *it;
			Grammar newGrammar = node->accept(*this);
			if (isGeneralized) {
				plusNode->replaceGrammarAt(newGrammar, node);
				break;
			}
		}
		return {};
	}

	std::vector<std::string>
	getRepResiduals(std::string sub1, std::string sub2, std::string sub3) {
		std::string res1, res2;
		res1.append(sub1).append(sub3);
		res2.append(sub1).append(sub2).append(sub2).append(sub3);
		return {res1, res2};
	}

	Grammar
	visit(RepNode* repNode) {
		if (isGeneralized) { return {}; }
		isGeneralized = true;
		PrintVisitor ps;
		std::cout << "Generalizing repNode ";
		repNode->accept(ps);
		std::cout << std::endl;
		
		GetContextVisitor getContextVisitor(rootGrammar, repNode);
		Context context = getContextVisitor.getContext();
		
		std::cout << "context = (" << context.left << ", " << context.right << ")" << std::endl;
		// exit(-1);
		std::string alpha = repNode->getTerminal();
		std::string sub1, sub2, sub3;
		int length = alpha.length();
		std::cout << "decomposing \'" << alpha << "\'" << std::endl;
		// primary priority for shorter sub1
		for (int i = 0; i < length; i++) {
			// sedondary priority for longer sub2
			for (int j = length; j > i; j--) { // todo: debug explicitly
				sub1 = alpha.substr(0, i);
				sub2 = alpha.substr(i, j-i);
				sub3 = alpha.substr(j, length - j);
				std::vector<std::string> residuals = getRepResiduals(sub1, sub2, sub3);

				std::cout << "\'" <<  sub1 << "\'" << std::endl;
				std::cout << "\'" <<  sub2 << "\'" << std::endl;
				std::cout << "\'" <<  sub3 << "\'" << std::endl;

				bool isPrecisionPreserving = true;
				for (std::string res : residuals) {
					std::string check;
					check.append(context.left).append(res).append(context.right);
					if (!oracle.query(check)) {
						isPrecisionPreserving = false;
						break;
					}
				}

				if (isPrecisionPreserving) {
					TerminalNode* decmp1 = new TerminalNode(sub1);
					StarNode* 		decmp2 = new StarNode();
					RepNode* 			decmp3 = new RepNode(sub3);
					decmp2->addChild(new AltNode(sub2));

					Grammar candidate;
					if (sub1.length() > 0) { candidate.push_back(decmp1); }
					// sub2 is never empty
					candidate.push_back(decmp2);
					if (sub3.length() > 0) { candidate.push_back(decmp3); }

					std::cout << "all checks pass!" << std::endl;
					PrintVisitor ps;
					std::cout << "candidate is ";
					for (Node* node : candidate) {
						node->accept(ps);
					}
					std::cout << std::endl;

					if (!isPreviouslyConsidered(candidate)) {
						blackListCandidate(candidate);
						return candidate;
					}
				}
			}
		}
		std::cout << "exhausted all candidates" << std::endl;
		// exit(-1);
		// last resort generalization
		return {new TerminalNode(alpha)};;
	}

	Grammar
	visit(AltNode* altNode) {
		if (isGeneralized) { return {}; }
		isGeneralized = true;
		PrintVisitor ps;
		std::cout << "Generalizing altNode ";
		altNode->accept(ps);
		std::cout << std::endl;

		GetContextVisitor getContextVisitor(rootGrammar, altNode);
		Context context = getContextVisitor.getContext();

		std::cout << "context = (" << context.left << ", " << context.right << ")" << std::endl;
		std::string alpha = altNode->getTerminal();
		std::string sub1, sub2;
		int length = alpha.length();
		std::cout << "decomposing \'" << alpha << "\'" << std::endl;
		// priority for shorter sub1
		for (int i = 1; i < length; i++) {
			sub1 = alpha.substr(0, i);
			sub2 = alpha.substr(i, length-i);
			std::vector<std::string> residuals = {sub1, sub2};

				std::cout << "\'" <<  sub1 << "\'" << std::endl;
				std::cout << "\'" <<  sub2 << "\'" << std::endl;

				bool isPrecisionPreserving = true;
			for (std::string res : residuals) {
				std::string check;
				check.append(context.left).append(res).append(context.right);
				if (!oracle.query(check)) {
					isPrecisionPreserving = false;
					break;
				}
			}

			if (isPrecisionPreserving) {
				RepNode* rep	 = new RepNode(sub1);
				AltNode* alt	 = new AltNode(sub2);
				PlusNode* plus = new PlusNode();
				plus->addChild(rep);
				plus->addChild(alt);

				Grammar candidate;
				candidate.push_back(plus);

				std::cout << "all checks pass!" << std::endl;
				std::cout << "candidate is ";
				for (Node* node : candidate) {
					node->accept(ps);
				}
				std::cout << std::endl;

				if (!isPreviouslyConsidered(candidate)) {
					blackListCandidate(candidate);
					return candidate;
				}
			}
		}
		std::cout << "exhausted all candidates" << std::endl;
		// last resort generalization
		return {new RepNode(alpha)};;
	}

	std::string getGrammarString(Grammar& grammar) {
		ToStringVisitor tsv;
		for (Node* node : grammar) {
			node->accept(tsv);
		}
		return tsv.getString();
	}

	bool 
	isPreviouslyConsidered(Grammar& grammar) {
		std::string str = getGrammarString(grammar);
		return considered.count(str);
	}

	void
	blackListCandidate(Grammar& candidate) {
		std::string str = getGrammarString(candidate);
		considered.insert(str);
	}

	bool checkGeneralized() { return isGeneralized; }

	void 
	init(Grammar& grammar) {
		rootGrammar = grammar;
		isGeneralized = false;
	}

private:
	Grammar rootGrammar;
	bool isGeneralized = false;
	Oracle& oracle;
	std::unordered_set<std::string> considered;
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
		GeneralizeVisitor generalizeVisitor(oracle);
		PrintVisitor ps;
		std::cout << "old grammar: [";
		for (Node* node : grammar) {
			node->accept(ps);
			std::cout << ", ";
		}
		std::cout << " ]" << std::endl;
		int count = 0;
		while (isGeneralized) {
			isGeneralized = false;
			generalizeVisitor.init(grammar);
			// todo: context must be generated at language level not withing generalization
			for (int i = grammar.size() - 1; i >= 0; i--) {
				Node* toGeneralize = grammar[i];
				Grammar newGrammar = toGeneralize->accept(generalizeVisitor);

				if (generalizeVisitor.checkGeneralized()) {
					isGeneralized = true;
					// if generalized node was a root,
					// must manually update the grammar
					// else replacement happens in GeneralizeVisitor
					if (newGrammar.size() > 0) {
						auto it  = std::find (grammar.begin(), grammar.end(), toGeneralize);
						auto loc = grammar.erase(it);
						grammar.insert(loc, newGrammar.begin(), newGrammar.end());
					}
					break;
				}
				else {
					std::cout << "nothing to generalize" << std::endl;
				}
			}
			std::cout << "new grammar: [";
			for (Node* node : grammar) {
				node->accept(ps);
				std::cout << ", ";
			}
			std::cout << " ]" << std::endl;
			if (count == 4) { exit(-1); }
			count++;
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
		std::cout << "checking \'" << check << "\'";
		try{
			// create wrapper  
			// because valid xml must have a root node
			std::string wrapper = "<i> ";
			wrapper.append(check).append("</i>");
			parser.parse_memory(wrapper);
		}
		catch(const std::exception& e) {
			std::cout << " -  failed" << std::endl;
			return false;
		}
		return true;
	}

	private:
	xmlpp::DomParser parser;
};

int main() {
	std::string seed = "<a>hi</a>";
	XMLOracle oracle;
	GrammarSynthesizer gs(seed, oracle);
	Grammar grammar = gs.synthesize();

	StarNode star;
	TerminalNode terminal("test");
	star.addChild(&terminal);
	
	PrintVisitor ps;
	// star.accept(ps);
	for (Node* node : grammar) {
		node->accept(ps);
	}

	// std::cout << oracle.query(seed);
	return 0;
}