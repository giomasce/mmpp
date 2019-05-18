
import gapt.provers.prover9.Prover9
import gapt.provers.eprover.EProver
import gapt.provers.spass.SPASS
import gapt.provers.metis.Metis
import gapt.proofs.lk.transformations.LKToND
import gapt.proofs.expansion.ExpansionProofToLK
import gapt.proofs.expansion.deskolemizeET
import gapt.formats.tptp.TptpImporter
import gapt.expr.stringInterpolationForExpressions
import gapt.expr._
import gapt.proofs.Sequent
import gapt.proofs.nd._
import gapt.expr.constants._

object GAPTInterface {
  def decomposeApp(f: Expr): (Const, Vector[Expr]) = {
    f match {
      case x: Const => {
        (x, Vector.empty)
      }
      case x: App => {
        val y = decomposeApp(x.function)
        (y._1, y._2 :+ x.arg)
      }
    }
  }

  def decomposeFormula(f: Expr, depth: Int = 0): Unit = {
    f match {
      case TopC() => {
        print("true")
      }
      case BottomC() => {
        print("false")
      }
      case x: Var => {
        print(s"var ${x.name}")
      }
      case _ => {
        val app_data = decomposeApp(f)
        app_data._1 match {
          case ForallC(_) => {
            app_data._2 match {
              case Vector(y) => y match {
                case z: Abs => {
                  print("forall ")
                  decomposeFormula(z.variable)
                  print(" ")
                  decomposeFormula(z.term)
                }
              }
            }
          }
          case ExistsC(_) => {
            app_data._2 match {
              case Vector(y) => y match {
                case z: Abs => {
                  print("exists ")
                  decomposeFormula(z.variable)
                  print(" ")
                  decomposeFormula(z.term)
                }
              }
            }
          }
          case ImpC() => {
            app_data._2 match {
              case Vector(y, z) => {
                print("imp ")
                decomposeFormula(y)
                print(" ")
                decomposeFormula(z)
              }
            }
          }
          case AndC() => {
            app_data._2 match {
              case Vector(y, z) => {
                print("and ")
                decomposeFormula(y)
                print(" ")
                decomposeFormula(z)
              }
            }
          }
          case OrC() => {
            app_data._2 match {
              case Vector(y, z) => {
                print("or ")
                decomposeFormula(y)
                print(" ")
                decomposeFormula(z)
              }
            }
          }
          case NegC() => {
            app_data._2 match {
              case Vector(y) => {
                print("not ")
                decomposeFormula(y)
              }
            }
          }
          case _ => {
            if (app_data._1.name == "=") {
              app_data._2 match {
                case Vector(y, z) => {
                  print("equal ")
                  decomposeFormula(y)
                  print(" ")
                  decomposeFormula(z)
                }
              }
            } else {
              print(s"unint ${app_data._1.name} ${app_data._2.length}")
              for (f2 <- app_data._2) {
                print(" ")
                decomposeFormula(f2)
              }
            }
          }
        }
      }
    }
  }

  def decomposeSequent(s: Sequent[Formula], depth: Int = 0): Unit = {
    //print("  " * depth)
    print(s"${s.antecedent.length}")
    for (ant <- s.antecedent) {
      print(" ")
      decomposeFormula(ant, depth+1)
    }
    //print("  " * depth)
    print(s" ${s.succedent.length}")
    for (suc <- s.succedent) {
      print(" ")
      decomposeFormula(suc, depth+1)
    }
  }

  def decomposeProof(p: NDProof, depth: Int = 0): Unit = {
    decomposeSequent(p.endSequent, depth)
    p match {
      case x: LogicalAxiom => {
        print(" LogicalAxiom ")
        decomposeFormula(x.A)
      }
      case x: WeakeningRule => {
        print(" Weakening ")
        decomposeFormula(x.formula)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x: ContractionRule => {
        print(s" Contraction ${x.aux1.toInt} ${x.aux2.toInt} ")
        decomposeProof(x.subProof)
      }
      case x: AndIntroRule => {
        print(" AndIntro ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      case x: AndElim1Rule => {
        print(" AndElim1 ")
        decomposeProof(x.subProof)
      }
      case x: AndElim2Rule => {
        print(" AndElim2 ")
        decomposeProof(x.subProof)
      }
      case x : OrIntro1Rule => {
        print(" OrIntro1 ")
        decomposeFormula(x.rightDisjunct)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x : OrIntro2Rule => {
        print(" OrIntro2 ")
        decomposeFormula(x.leftDisjunct)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x : OrElimRule => {
        print(s" OrElim ${x.aux1.toInt} ${x.aux2.toInt} ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.middleSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      case x: NegElimRule => {
        print(" NegElim ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      case x: ImpIntroRule => {
        print(s" ImpIntro ${x.aux.toInt} ")
        decomposeProof(x.subProof)
      }
      case x: ImpElimRule => {
        print(" ImpElim ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      /*case x: TopIntroRule => {
        print(" TopIntro")
      }*/
      case x: BottomElimRule => {
        print(" BottomElim ")
        decomposeFormula(x.mainFormula)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x: ForallIntroRule => {
        print(" ForallIntro ")
        decomposeFormula(x.quantifiedVariable)
        print(" ")
        decomposeFormula(x.eigenVariable)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x: ForallElimRule => {
        print(" ForallElim ")
        decomposeFormula(x.term)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x: ExistsIntroRule => {
        print(" ExistsIntro ")
        decomposeFormula(x.A)
        print(" ")
        decomposeFormula(x.v)
        print(" ")
        decomposeFormula(x.term)
        print(" ")
        decomposeProof(x.subProof)
      }
      case x: ExistsElimRule => {
        print(s" ExistsElim ${x.aux.toInt} ")
        decomposeFormula(x.eigenVariable)
        print(" ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      case x : EqualityIntroRule => {
        print(" EqualityIntro ")
        decomposeFormula(x.t)
      }
      case x : EqualityElimRule => {
        print(" EqualityElim ")
        decomposeFormula(x.variablex)
        print(" ")
        decomposeFormula(x.formulaA)
        print(" ")
        decomposeProof(x.leftSubProof)
        print(" ")
        decomposeProof(x.rightSubProof)
      }
      case x: ExcludedMiddleRule => {
        print(s" ExcludedMiddle ${x.aux1.toInt} ${x.aux2.toInt} ")
        decomposeProof(x.leftSubProof, depth+1)
        print(" ")
        decomposeProof(x.rightSubProof, depth+1)
      }
      case _ => {
        throw new Exception("Unknown proof step " + p.getClass)
      }
    }
  }

  def main(args: Array[String]): Unit = {
    //val tptpProblem = TptpImporter.loadWithIncludes("/tmp/x.p")
    //val sequent = tptpProblem.toSequent
    //val sequentStr = """:- ((?x !y (f(x) <-> f(y))) <-> ((?x (g(x)) <-> (!y g(y))))) <->
    //             ((?x !y (g(x) <-> g(y))) <-> ((?x (f(x)) <-> (!y f(y)))))"""
    //val sequentStr = ":- ?x (f(x) -> !y f(y))"
    //val sequentStr = "!x f(x) -> g :- ?x (f(x) -> g)"
    //val sequentStr = "(f -> g) | (f -> h) :- f -> (g | h)"
    //val sequentStr = "x = y, f(x) :- f(y)"
    val sequentStr = args(0)
    val sequent = stringInterpolationForExpressions(new StringContext(sequentStr)).fos()
    //val sequent : FOLFormula = Top()
    // decomposeSequent(sequent)
    // println()
    System.err.println("Target sequent:")
    System.err.println(sequent)
    val expProof = Prover9.getExpansionProof(sequent).get
    //val expProof = EProver.getExpansionProof(sequent).get
    //val expProof = SPASS.getExpansionProof(sequent).get
    //val expProof = Metis.getExpansionProof(sequent).get
    // println("Expansion-tree proof:")
    // println(expProof)
    val deskProof = deskolemizeET(expProof)
    // println("Deskolemized proof:")
    // println(deskProof)
    val lkProof = ExpansionProofToLK(deskProof) match {
      case Right(x) => x
      case Left(_) => throw new Exception("Exception")
    }
    val ndProof = LKToND(lkProof)
    //System.err.println("ND proof:")
    //System.err.println(ndProof)
    //println("Now let us try to understand what is going on!")
    val formula = ndProof.endSequent.succedent.last
    //println(ndProof.getClass)
    decomposeProof(ndProof)
    println()
  }
}
