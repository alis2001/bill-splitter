import React, { useState, useEffect, useContext } from 'react';
import { 
  View, Text, StyleSheet, ScrollView, TouchableOpacity, 
  Alert, RefreshControl, Animated 
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Ionicons } from '@expo/vector-icons';
import { AuthContext } from '../../context/AuthContext';
import { eventsAPI } from '../../services/apiService';

const EventDetailsScreen = ({ navigation, route }) => {
  const { event: initialEvent, isNew } = route.params;
  const { user } = useContext(AuthContext);
  const [event, setEvent] = useState(initialEvent);
  const [expenses, setExpenses] = useState([]);
  const [participants, setParticipants] = useState([]);
  const [settlements, setSettlements] = useState(null);
  const [loading, setLoading] = useState(!isNew);
  const [refreshing, setRefreshing] = useState(false);
  const fadeAnim = new Animated.Value(0);

  useEffect(() => {
    if (isNew) {
      Animated.timing(fadeAnim, {
        toValue: 1,
        duration: 500,
        useNativeDriver: true,
      }).start();
    }
    loadEventData();
  }, []);

  const loadEventData = async () => {
    try {
      const [expensesRes, participantsRes, settlementsRes] = await Promise.all([
        eventsAPI.getExpenses(event.id),
        eventsAPI.getParticipants(event.id),
        eventsAPI.getSettlements(event.id)
      ]);

      if (expensesRes.success) setExpenses(expensesRes.data.expenses || []);
      if (participantsRes.success) setParticipants(participantsRes.data.participants || []);
      if (settlementsRes.success) setSettlements(settlementsRes.data);
    } catch (error) {
      Alert.alert('Error', 'Failed to load event data');
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  };

  const handleRefresh = () => {
    setRefreshing(true);
    loadEventData();
  };

  const totalExpenses = expenses.reduce((sum, exp) => sum + exp.amount, 0);
  const isCreator = event.creator_id === user?.id;
  const myBalance = settlements?.balances?.[user?.id] || 0;

  const getEventIcon = () => {
    const icons = {
      restaurant: 'restaurant',
      travel: 'airplane',
      shopping: 'bag',
      entertainment: 'film',
      utilities: 'home',
      other: 'receipt'
    };
    return icons[event.event_type] || 'receipt';
  };

  const renderQuickActions = () => (
    <View style={styles.quickActions}>
      <TouchableOpacity 
        style={styles.actionButton}
        onPress={() => navigation.navigate('AddExpense', { event })}
      >
        <Ionicons name="add-circle" size={20} color="#FFFFFF" />
        <Text style={styles.actionText}>Add Expense</Text>
      </TouchableOpacity>
      
      {isCreator && (
        <TouchableOpacity 
          style={[styles.actionButton, styles.secondaryAction]}
          onPress={() => Alert.alert('Feature Coming Soon', 'Invite friends functionality')}
        >
          <Ionicons name="person-add" size={20} color="#1A365D" />
          <Text style={[styles.actionText, styles.secondaryText]}>Invite</Text>
        </TouchableOpacity>
      )}
    </View>
  );

  const renderExpenseItem = (expense) => (
    <TouchableOpacity key={expense.id} style={styles.expenseCard} activeOpacity={0.7}>
      <View style={styles.expenseInfo}>
        <Text style={styles.expenseDescription}>{expense.description}</Text>
        <Text style={styles.expensePayer}>
          Paid by {expense.payer?.name || 'Unknown'}
        </Text>
        <Text style={styles.expenseDate}>
          {new Date(expense.expense_date).toLocaleDateString()}
        </Text>
      </View>
      <View style={styles.expenseAmount}>
        <Text style={styles.amountText}>${expense.amount.toFixed(2)}</Text>
        <View style={[styles.splitIndicator, 
          { backgroundColor: expense.split_type === 'equal' ? '#38A169' : '#45B7D1' }
        ]}>
          <Text style={styles.splitText}>
            {expense.split_type === 'equal' ? 'Equal' : 'Custom'}
          </Text>
        </View>
      </View>
    </TouchableOpacity>
  );

  if (loading) {
    return (
      <SafeAreaView style={styles.container}>
        <View style={styles.loadingContainer}>
          <Ionicons name="hourglass" size={48} color="#1A365D" />
          <Text style={styles.loadingText}>Loading event...</Text>
        </View>
      </SafeAreaView>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <TouchableOpacity onPress={() => navigation.goBack()}>
          <Ionicons name="arrow-back" size={24} color="#2D3748" />
        </TouchableOpacity>
        <View style={styles.headerTitle}>
          <Ionicons name={getEventIcon()} size={20} color="#1A365D" />
          <Text style={styles.title} numberOfLines={1}>{event.name}</Text>
        </View>
        <TouchableOpacity onPress={() => {}}>
          <Ionicons name="ellipsis-horizontal" size={24} color="#2D3748" />
        </TouchableOpacity>
      </View>

      <ScrollView 
        style={styles.content}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={handleRefresh} />
        }
        showsVerticalScrollIndicator={false}
      >
        {/* Summary Card */}
        <Animated.View style={[styles.summaryCard, { opacity: fadeAnim }]}>
          <View style={styles.summaryHeader}>
            <View>
              <Text style={styles.totalAmount}>${totalExpenses.toFixed(2)}</Text>
              <Text style={styles.totalLabel}>Total Spent</Text>
            </View>
            <View style={styles.balanceInfo}>
              <Text style={[
                styles.balanceAmount,
                { color: myBalance >= 0 ? '#38A169' : '#E53E3E' }
              ]}>
                {myBalance >= 0 ? '+' : ''}${Math.abs(myBalance).toFixed(2)}
              </Text>
              <Text style={styles.balanceLabel}>
                {myBalance >= 0 ? 'You\'re owed' : 'You owe'}
              </Text>
            </View>
          </View>
          
          <View style={styles.participantInfo}>
            <Ionicons name="people" size={16} color="#718096" />
            <Text style={styles.participantText}>
              {participants.length + 1} people
            </Text>
          </View>
        </Animated.View>

        {renderQuickActions()}

        {/* Expenses Section */}
        <View style={styles.section}>
          <View style={styles.sectionHeader}>
            <Text style={styles.sectionTitle}>Expenses</Text>
            <Text style={styles.expenseCount}>{expenses.length}</Text>
          </View>
          
          {expenses.length === 0 ? (
            <View style={styles.emptyState}>
              <Ionicons name="receipt-outline" size={48} color="#A0AEC0" />
              <Text style={styles.emptyTitle}>No expenses yet</Text>
              <Text style={styles.emptySubtitle}>
                {isNew ? 'Start by adding your first expense!' : 'Tap the + button to add an expense'}
              </Text>
            </View>
          ) : (
            expenses.map(renderExpenseItem)
          )}
        </View>

        {/* Settlement Button */}
        {expenses.length > 0 && settlements && (
          <TouchableOpacity 
            style={styles.settleButton}
            onPress={() => navigation.navigate('Settle', { event, settlements })}
          >
            <Ionicons name="calculator" size={20} color="#FFFFFF" />
            <Text style={styles.settleButtonText}>View Settlements</Text>
          </TouchableOpacity>
        )}
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E2E8F0',
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
  },
  headerTitle: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    justifyContent: 'center',
  },
  title: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
    marginLeft: 8,
  },
  content: { flex: 1, padding: 16 },
  summaryCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 16,
    padding: 20,
    marginBottom: 16,
    elevation: 3,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
  },
  summaryHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  totalAmount: {
    fontSize: 28,
    fontWeight: '700',
    color: '#2D3748',
  },
  totalLabel: {
    fontSize: 14,
    color: '#718096',
    marginTop: 2,
  },
  balanceAmount: {
    fontSize: 18,
    fontWeight: '600',
    textAlign: 'right',
  },
  balanceLabel: {
    fontSize: 12,
    color: '#718096',
    textAlign: 'right',
    marginTop: 2,
  },
  participantInfo: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  participantText: {
    fontSize: 14,
    color: '#718096',
    marginLeft: 4,
  },
  quickActions: {
    flexDirection: 'row',
    marginBottom: 24,
    gap: 12,
  },
  actionButton: {
    flex: 1,
    backgroundColor: '#1A365D',
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 14,
    borderRadius: 12,
    elevation: 2,
  },
  secondaryAction: {
    backgroundColor: '#FFFFFF',
    borderWidth: 1,
    borderColor: '#1A365D',
  },
  actionText: {
    color: '#FFFFFF',
    fontWeight: '600',
    marginLeft: 8,
    fontSize: 14,
  },
  secondaryText: {
    color: '#1A365D',
  },
  section: { marginBottom: 24 },
  sectionHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#2D3748',
  },
  expenseCount: {
    fontSize: 14,
    color: '#718096',
    backgroundColor: '#F7FAFC',
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 10,
  },
  expenseCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    flexDirection: 'row',
    alignItems: 'center',
    elevation: 1,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
  },
  expenseInfo: { flex: 1 },
  expenseDescription: {
    fontSize: 16,
    fontWeight: '500',
    color: '#2D3748',
    marginBottom: 4,
  },
  expensePayer: {
    fontSize: 14,
    color: '#718096',
    marginBottom: 2,
  },
  expenseDate: {
    fontSize: 12,
    color: '#A0AEC0',
  },
  expenseAmount: { alignItems: 'flex-end' },
  amountText: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
    marginBottom: 4,
  },
  splitIndicator: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 10,
  },
  splitText: {
    fontSize: 10,
    color: '#FFFFFF',
    fontWeight: '500',
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    fontSize: 16,
    color: '#718096',
    marginTop: 12,
  },
  emptyState: {
    alignItems: 'center',
    paddingVertical: 32,
  },
  emptyTitle: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
    marginTop: 12,
    marginBottom: 4,
  },
  emptySubtitle: {
    fontSize: 14,
    color: '#718096',
    textAlign: 'center',
    lineHeight: 20,
  },
  settleButton: {
    backgroundColor: '#38A169',
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 16,
    borderRadius: 12,
    marginTop: 16,
    marginBottom: 32,
    elevation: 3,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
  },
  settleButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
    marginLeft: 8,
  },
});

export default EventDetailsScreen;