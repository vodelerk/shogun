/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Heiko Strathmann, Saurabh Mahindre, Sergey Lisitsyn, 
 *          Soeren Sonnenburg, Fernando Iglesias, Evgeniy Andreev, 
 *          Chiyuan Zhang, Olivier NGuyen, Thoralf Klein
 */

#include <shogun/base/range.h>
#include <shogun/labels/BinaryLabels.h>
#include <shogun/labels/DenseLabels.h>
#include <shogun/lib/SGVector.h>
#include <shogun/mathematics/Statistics.h>

using namespace shogun;

CBinaryLabels::CBinaryLabels() : CDenseLabels()
{
}

CBinaryLabels::CBinaryLabels(int32_t num_labels) : CDenseLabels(num_labels)
{
}

#if !defined(SWIGJAVA) && !defined(SWIGCSHARP)
CBinaryLabels::CBinaryLabels(SGVector<int32_t> src) : CDenseLabels()
{
	SGVector<float64_t> values(src.vlen);
	for (int32_t i = 0; i < values.vlen; i++)
	{
		values[i] = src[i];
	}
	set_int_labels(src);
	set_values(values);
}

CBinaryLabels::CBinaryLabels(SGVector<int64_t> src) : CDenseLabels()
{
	SGVector<float64_t> values(src.vlen);
	for (int32_t i = 0; i < values.vlen; i++)
	{
		values[i] = src[i];
	}
	set_int_labels(src);
	set_values(values);
}
#endif

CBinaryLabels::CBinaryLabels(SGVector<float64_t> src, float64_t threshold) : CDenseLabels()
{
	SGVector<float64_t> labels(src.vlen);
	for (int32_t i = 0; i < labels.vlen; i++)
	{
		labels[i] = src[i] >= threshold ? +1.0 : -1.0;
	}
	set_labels(labels);
	set_values(src);
}

CBinaryLabels::CBinaryLabels(CFile * loader) : CDenseLabels(loader)
{
}

void CBinaryLabels::ensure_valid(const char * context)
{
	CDenseLabels::ensure_valid(context);
	bool found_plus_one = false;
	bool found_minus_one = false;

	int32_t subset_size = get_num_labels();
	for (int32_t i = 0; i < subset_size; i++)
	{
		int32_t real_i = m_subset_stack->subset_idx_conversion(i);
		if (m_labels[real_i] == +1.0)
		{
			found_plus_one = true;
		}
		else if (m_labels[real_i] == -1.0)
		{
			found_minus_one = true;
		}
		else
		{
			SG_ERROR(
			        "%s%s%s::ensure_valid(): Not a two class labeling label[%d]=%f (only +1/-1 "
			        "allowed)\n", context ? context : "",
			        context ? ": " : "", get_name(), i, m_labels[real_i]);
		}
	}

	if (!found_plus_one)
	{
		SG_WARNING(
		        "%s%s%s::ensure_valid(): Not a two class labeling - no positively labeled examples found\n",
		        context ? context : "", context ? ": " : "", get_name());
	}

	if (!found_minus_one)
	{
		SG_WARNING(
		        "%s%s%s::ensure_valid): Not a two class labeling - no negatively labeled examples found\n",
		        context ? context : "", context ? ": " : "", get_name());
	}
}

ELabelType CBinaryLabels::get_label_type() const
{
	return LT_BINARY;
}

void CBinaryLabels::scores_to_probabilities(float64_t a, float64_t b)
{
	SG_DEBUG("entering CBinaryLabels::scores_to_probabilities()\n")

	REQUIRE(m_current_values.vector, "%s::scores_to_probabilities() requires "
	        "values vector!\n", get_name());

	if (a == 0 && b == 0)
	{
		CStatistics::SigmoidParamters params =
		        CStatistics::fit_sigmoid(m_current_values);
		a = params.a;
		b = params.b;
	}

	SG_DEBUG("using sigmoid: a=%f, b=%f\n", a, b)

	/* now the sigmoid is fitted, convert all values to probabilities */
	for (index_t i = 0; i < m_current_values.vlen; ++i)
	{
		float64_t fApB = m_current_values[i] * a + b;
		m_current_values[i] = fApB >= 0
		                          ? std::exp(-fApB) / (1.0 + std::exp(-fApB))
		                          : 1.0 / (1 + std::exp(fApB));
	}

	SG_DEBUG("leaving CBinaryLabels::scores_to_probabilities()\n")
}

CLabels* CBinaryLabels::shallow_subset_copy()
{
	CLabels* shallow_copy_labels=NULL;
	SGVector<float64_t> shallow_copy_vector(m_labels);
	shallow_copy_labels=new CBinaryLabels(m_labels.size());
	SG_REF(shallow_copy_labels);

	((CDenseLabels*) shallow_copy_labels)->set_labels(shallow_copy_vector);
	if (m_subset_stack->has_subsets())
		shallow_copy_labels->add_subset(m_subset_stack->get_last_subset()->get_subset_idx());

	return shallow_copy_labels;
}

namespace shogun
{
	Some<CBinaryLabels> binary_from_binary(CBinaryLabels* orig)
	{
		return Some<CBinaryLabels>::from_raw(orig);
	}

	Some<CBinaryLabels> binary_from_dense(CDenseLabels* orig)
	{
		auto labels = orig->get_labels();
		for (auto i : range(labels.vlen))
		{
			if (!CMath::fequals(labels[i], 1.0, 0.0) &&
			    !CMath::fequals(labels[i], -1.0, 0.0))
			{
				SG_SERROR(
				    "Label[%d] is %f, but must be either +1.0 or -1.0.\n", i,
				    labels[i])
			}
		}
		auto result = new CBinaryLabels();
		result->set_labels(labels);
		result->set_values(orig->get_values());
		return Some<CBinaryLabels>::from_raw(result);
	}

	Some<CBinaryLabels> binary_labels(CLabels* orig)
	{
		REQUIRE(orig, "No labels provided.\n");
		try
		{
			switch (orig->get_label_type())
			{
			case LT_BINARY:
				return binary_from_binary(orig->as<CBinaryLabels>());
			case LT_DENSE_GENERIC:
				return binary_from_dense(orig->as<CDenseLabels>());
			default:
				SG_SNOTIMPLEMENTED
			}
		}
		catch (const ShogunException& e)
		{
			SG_SERROR(
			    "Cannot convert %s to binary labels: \n", e.what(),
			    orig->get_name());
		}

		return Some<CBinaryLabels>::from_raw(nullptr);
	}
} // namespace shogun
